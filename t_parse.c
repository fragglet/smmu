/***************************** FraggleScript ******************************/
                // Copyright(C) 1999 Simon Howard 'Fraggle' //
//
// Parsing.
//
// Takes lines of code, or groups of lines and runs them.
//

/* includes ************************/

#include <stdio.h>
#include "c_io.h"
#include "z_zone.h"
#include <stdarg.h>
#include "doomtype.h"
#include "s_sound.h"

#include "t_main.h"
#include "t_parse.h"
#include "t_prepro.h"
#include "t_spec.h"
#include "t_oper.h"
#include "t_vari.h"
#include "t_func.h"

void parse_script();
svalue_t evaluate_expression(int start, int stop);

char *tokens[T_MAXTOKENS];
tokentype_t tokentype[T_MAXTOKENS];
int num_tokens = 0;
int script_debug = false;

script_t *current_script;       // the current script
svalue_t nullvar = { svt_int,  {0} };      // null var for empty return
int killscript;         // when set to true, stop the script quickly
section_t *prev_section;       // the section from the previous statement

/************ Divide into tokens **************/

char *linestart;        // start of line
char *rover;            // current point reached in script

        // inline for speed
#define isnum(c) ( (c)>='0' && (c)<='9' )
        // isop: is an 'operator' character, eg '=', '%'
#define isop(c)   !( ( (c)<='Z' && (c)>='A') || ( (c)<='z' && (c)>='a') || \
                     ( (c)<='9' && (c)>='0') || ( (c)=='_') )

        // for simplicity:
#define tt (tokentype[num_tokens-1])
#define tok (tokens[num_tokens-1])

section_t *current_section; // the section (if any) found in parsing the line
int bracetype;              // bracket_open or bracket_close
void add_char(char c);

// next_token: end this token, go onto the next

void next_token()
{
        if(tok[0] || tt==string)
        {
                num_tokens++;
                tokens[num_tokens-1] = tokens[num_tokens-2]
                                 + strlen(tokens[num_tokens-2]) + 1;
                tok[0] = 0;
        }

                // get to the next token, ignoring spaces, newlines,
                // useless chars, comments etc
        while(1)
        {
                        // empty whitespace
                if(*rover && (*rover==' ' || *rover<32))
                {
                    while((*rover==' ' || *rover<32) && *rover) rover++;
                }
                        // end-of-script?
                if(!*rover)
                {
                        if(tokens[0][0])
                        {
                                C_Printf("%s %i %i\n", tokens[0], rover-current_script->data, current_script->len);
                                // line contains text, but no semicolon
                                script_error("missing ';'\n");
                        }
                                // empty line, end of command-list
                        return;
                }
                        // 11/8 comments moved to new preprocessor

                break;  // otherwise
        }

        if(*rover == '{' || *rover == '}')
        {
                if(*rover == '{')
                {
                    bracetype = bracket_open;
                    current_section = find_section_start(rover);
                }
                else            // closing brace
                {
                    bracetype = bracket_close;
                    current_section = find_section_end(rover);
                }
                if(!current_section)
                {
                        script_error("section not found!\n");
                        return;
                }
        }
        else if(*rover == ':')  // label
        {
                // ignore the label : reset
                num_tokens = 1;
                tokens[0][0] = 0; tt = name;
                rover++;        // ignore
        }
        else if(*rover == '\"')
        {
                tt = string;
                if(tokentype[num_tokens-2] == string)
                        num_tokens--;   // join strings
                rover++;
        }
        else
        {
                tt = isop(*rover) ? operator :
                     isnum(*rover) ? number : name;
        }
}

// return an escape sequence (prefixed by a '\')
// do not use all C escape sequences

char escape_sequence(char c)
{
        if(c == 'n') return '\n';
        if(c == '\\') return '\\';
        if(c == '"') return '"';
        if(c == '?') return '?';
        if(c == 'a') return '\a'; // alert beep
        if(c == 't') return '\t'; //tab

        return c;
}

// add_char: add one character to the current token

void add_char(char c)
{
        char *out = tok + strlen(tok);

        *out++ = c;
        *out = 0;
}

        // second stage parsing.
        // go through the tokens and mark brackets as brackets
        // also mark function names, which are name tokens
        // followed by open brackets

void find_brackets()
{
        int i;

        for(i=0; i<num_tokens; i++)
        {
            if(tokentype[i] == operator)
            {
               if(tokens[i][0] == '(')
               {
                  tokentype[i] = bracket_open;
                  if(i && tokentype[i-1] == name)
                    tokentype[i-1] = function;
               }
               else
                  if(tokens[i][0] == ')')
                    tokentype[i] = bracket_close;
            }
        }
}

// get_tokens.
// Take a string, break it into tokens.

        // individual tokens are stored inside the tokens[] array
        // which is created by run_line. tokentype is also used
        // to hold the type for each token:

        //   name: a piece of text which starts with an alphabet letter.
        //         probably a variable name. Some are converted into
        //         function types later on in find_brackets
        //   number: a number. like '12' or '1337'
        //   operator: an operator such as '&&' or '+'. All FraggleScript
        //             operators are either one character, or two character
        //             (if 2 character, 2 of the same char or ending in '=')
        //   string: a text string that was enclosed in quote "" marks in
        //           the original text
        //   unset: shouldn't ever end up being set really.
        //   bracket_open, bracket_close : open and closing () brackets
        //   function: a function name (found in second stage parsing)


void get_tokens(char *s)
{
        rover = s;
        num_tokens = 1;
        tokens[0][0] = 0; tt = name;

        current_section = NULL;   // default to no section found

        next_token();
        linestart = rover;      // save the start

        if(*rover)
        while(1)
        {
             if(killscript) return;
             if(current_section)
             {
                        // a { or } section brace has been found
                  break;        // stop parsing now
             }
             else if(tt != string)
             {
                if(*rover == ';') break;     // check for end of command ';'
             }

             switch(tt)
             {
                  case unset:
                  case string:
                    while(*rover != '\"')     // dedicated loop for speed
                    {
                       if(*rover == '\\')       // escape sequences
                       {
                            rover++;
                            add_char(escape_sequence(*rover));
                       }
                       else
                            add_char(*rover);
                       rover++;
                    }
                    rover++;
                    next_token();       // end of this token
                    continue;

                  case operator:
                        // all 2-character operators either end in '=' or
                        // are 2 of the same character
                        // do not allow 2-characters for brackets '(' ')'
                        // which are still being considered as operators

                        // operators are only 2-char max, do not need
                        // a seperate loop

                    if((*tok && *rover != '=' && *rover!=*tok) ||
                        *tok == '(' || *tok == ')')
                    {
                          // end of operator
                        next_token();
                        continue;
                    }
                    add_char(*rover);
                    break;

                  case number:  // same for number or name
                  case name:
                                // add the chars
                    while(!isop(*rover))        // dedicated loop
                            add_char(*rover++);
                    next_token();
                    continue;
                  default:
             }
             rover++;
        }
        
        //        // check for empty last token
        if(!tok[0])
        {
                num_tokens = num_tokens - 1;
        }

                        // now do second stage parsing
        find_brackets();

        rover++;
}


void print_tokens()	// DEBUG
        {
            int i;
            for(i=0; i<num_tokens; i++)
            {
                C_Printf("\n'%s' \t\t --", tokens[i]);
                switch(tokentype[i])
                {
                    case string: C_Printf("string");        break;
                    case operator: C_Printf("operator");    break;
                    case name: C_Printf("name");            break;
                    case number: C_Printf("number");        break;
                    case unset : C_Printf("duh");           break;
                    case bracket_open: C_Printf("open bracket"); break;
                    case bracket_close: C_Printf("close bracket"); break;
                    case function: C_Printf("function name"); break;
                }
            }
            C_Printf("\n");
            if(current_section)
                C_Printf("current section: offset %i\n",
                       (int)(current_section->start-current_script->data) );
        }


        // run_script
        //
        // the function called by t_script.c

void run_script(script_t *script)
{
                // set current script
        current_script = script;

                // start at the beginning of the script
        rover = current_script->data;

        parse_script(); // run it
}

void continue_script(script_t *script, char *continue_point)
{
        current_script = script;

                // continue from place specified
        rover = continue_point;

        parse_script(); // run 
}

void parse_script()
{
        char *token_alloc;      // allocated memory for tokens

                // check for valid rover
        if(rover < current_script->data
        || rover > current_script->data+current_script->len)
        {
                script_error("parse_script: trying to continue from point"
                             "outside script!\n");
                return;
        }

        killscript = false;     // dont kill the script straight away

                // allocate space for the tokens
        token_alloc = Z_Malloc(current_script->len + T_MAXTOKENS, PU_STATIC, 0);

        prev_section = NULL;  // clear it

        while(*rover)   // go through the script executing each statement
        {
                        // past end of script?
                if(rover > current_script->data+current_script->len)
                        break;

                        // reset the tokens before getting the next line
                tokens[0] = token_alloc;

                prev_section = current_section; // store from prev. statement

                        // get the line and tokens
                get_tokens(rover);
  
                if(killscript) break;

                if(!num_tokens)
                {
                        if(current_section)       // no tokens but a brace
                        {
                                   // possible } at end of loop:
                                   // refer to spec.c
                                spec_brace();
                        }

                        continue;  // continue to next statement
                }

                if(script_debug) print_tokens();   // debug
                run_statement();         // run the statement
        }
        Z_Free(token_alloc);

                // dont clear global vars!
        if(current_script->scriptnum != -1)
                clear_variables(current_script);        // free variables
}

void run_statement()
{
        // decide what to do with it

        // NB this stuff is a bit hardcoded:
        //    it could be nicer really but i'm
        //    aiming for speed

                // if() and while() will be mistaken for functions
                // during token processing
        if(tokentype[0] == function)
        {
                if(!strcmp(tokens[0], "if"))
                {
                    spec_if();
                    return;
                }
                else if(!strcmp(tokens[0], "while"))
                {
                    spec_while();
                    return;
                }
                else if(!strcmp(tokens[0], "for"))
                {
                    spec_for();
                    return;
                }
        }
        else if(tokentype[0] == name)
        {
                if(!strcmp(tokens[0], "int"))
                {
                    spec_int();
                    return;
                }
                else if(!strcmp(tokens[0], "string"))
                {
                    spec_string();
                    return;
                }
                else if(!strcmp(tokens[0], "script"))
                {
                    spec_script();
                    return;
                }
                // goto is handled as a function

                // NB "float" or other types could be added here
        }

                // just a plain expression
        evaluate_expression(0, num_tokens-1);
}

/***************** Evaluating Expressions ************************/

        // find a token, ignoring things in brackets        
int find_token(int start, int stop, char *value)
{
        int i;
        int bracketlevel = 0;

        for(i=start; i<=stop; i++)
        {
                        // use bracketlevel to check the number of brackets
                        // which we are inside
                bracketlevel += tokentype[i]==bracket_open ? 1 :
                                tokentype[i]==bracket_close ? -1 : 0;

                        // only check when we are not in brackets
                if(!bracketlevel && !strcmp(value, tokens[i]))
                        return i;
        }

        return -1;
}

        // go through tokens the same as find_token, but backwards
int find_token_backwards(int start, int stop, char *value)
{
        int i;
        int bracketlevel = 0;

        for(i=stop; i>=start; i--)      // check backwards
        {
                        // use bracketlevel to check the number of brackets
                        // which we are inside
                bracketlevel += tokentype[i]==bracket_open ? 1 :
                                tokentype[i]==bracket_close ? -1 : 0;

                        // only check when we are not in brackets
                if(!bracketlevel && !strcmp(value, tokens[i]))
                        return i;
        }

        return -1;
}

// simple_evaluate is used once evalute_expression gets to the level
// where it is evaluating just one token

// converts number tokens into svalue_ts and returns
// the same with string tokens
// name tokens are considered to be variables and
// attempts are made to find the value of that variable
// command tokens are executed (does not return a svalue_t)

extern svalue_t nullvar;

static svalue_t simple_evaluate(int n)
{
        svalue_t returnvar;
        svariable_t *var;

        switch(tokentype[n])
        {
            case string: returnvar.type = svt_string;
                         returnvar.value.s = tokens[n];
                         return returnvar;
            case number: returnvar.type = svt_int;
                         returnvar.value.i = atoi(tokens[n]);
                         return returnvar;
            case name:   var = find_variable(tokens[n]);
                         if(!var)
                         {
                            script_error("unknown variable '%s'\n", tokens[n]);
                            return nullvar;
                         }
                         else
                            return getvariablevalue(var);
            default: return nullvar;
        }
}

// pointless_brackets checks to see if there are brackets surrounding
// an expression. eg. "(2+4)" is the same as just "2+4"
//
// because of the recursive nature of evaluate_expression, this function is
// neccesary as evaluating expressions such as "2*(2+4)" will inevitably
// lead to evaluating "(2+4)"

static void pointless_brackets(int *start, int *stop)
{
        int bracket_level, i;

                // check that the start and end are brackets

        while(tokentype[*start] == bracket_open &&
               tokentype[*stop] == bracket_close)
        {

            bracket_level = 0;

                  // confirm there are pointless brackets..
                  // if they are, bracket_level will only get to 0
                  // at the last token
                  // check up to <*stop rather than <=*stop to ignore
                  // the last token
            for(i = *start; i<*stop; i++)
            {
                bracket_level += (tokentype[i] == bracket_open);
                bracket_level -= (tokentype[i] == bracket_close);
                if(bracket_level == 0) return;
            }

                // move both brackets in

            *start = *start + 1;
            *stop = *stop - 1;
        }
}

// evaluate_expresion is the basic function used to evaluate
// a FraggleScript expression.
// start and stop denote the tokens which are to be evaluated.
//
// works by recursion: it finds operators in the expression
// (checking for each in turn), then splits the expression into
// 2 parts, left and right of the operator found.
// The handler function for that particular operator is then
// called, which in turn calls evaluate_expression again to
// evaluate each side. When it reaches the level of being asked
// to evaluate just 1 token, it calls simple_evaluate

svalue_t evaluate_expression(int start, int stop)
{
        int n, count;

        if(killscript) return nullvar;  // killing the script

        // debug
//        C_Printf("evaluate_expression(%i, %i)\n", start, stop);

                // possible pointless brackets
        if(tokentype[start] == bracket_open
        && tokentype[stop] == bracket_close)
                pointless_brackets(&start, &stop);

        if(start == stop)       // only 1 thing to evaluate
        {
                return simple_evaluate(start);
        }

        // go through each operator in order of precedence

        for(count=0; count<num_operators; count++)
        {
                // check backwards for the token. it has to be
                // done backwards for left-to-right reading: eg so
                // 5-3-2 is (5-3)-2 not 5-(3-2)
           if( -1 != (n =
                (operators[count].direction==forward ?
                        find_token_backwards : find_token)
                (start, stop, operators[count].string)) )
           {
                // useful for debug:
//                C_Printf("operator %s: %i,%i,%i\n",
//                        operators[count].string, start, n, stop);

                        // call the operator function
                        // and evaluate this chunk of code
                return operators[count].handler(start, n, stop);
           }
        }

        if(tokentype[start] == function)
                return evaluate_function(start, stop);

        // error ?
        {        
                char tempstr[128]="";
                int i;

                for(i=start; i<=stop; i++)
                  sprintf(tempstr,"%s %s", tempstr, tokens[i]);
                script_error("couldnt evaluate expression: %s\n",tempstr);
                return nullvar;
        }

}

void script_error(char *s, ...)
{
        va_list args;
        char tempstr[128];

        va_start(args, s);

        if(killscript) return;  //already killing script

                // find the line number
        {
                int linenum = 1;
                char *temp;
                for(temp = current_script->data; temp<linestart; temp++)
                        if(*temp == '\n') linenum++;    // count EOLs
                if(current_script->scriptnum == -1)
                  C_Printf("global, %i: ", linenum);
                else
                  C_Printf("%i,%i: ", current_script->scriptnum, linenum);
        }

                // print the error
        vsprintf(tempstr, s, args);
        C_Printf(tempstr);

        // make a noise
        S_StartSound(NULL, sfx_pldeth);

        killscript = 1;
}
