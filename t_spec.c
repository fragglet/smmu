/***************************** FraggleScript ******************************/
                // Copyright(C) 1999 Simon Howard 'Fraggle' //
//
// 'Special' stuff
//
// if(), int statements, etc.
//

/* includes ************************/

#include <stdio.h>
#include "z_zone.h"

#include "t_parse.h"
#include "t_main.h"
#include "t_vari.h"

int find_token(int start, int stop, char *value);

        // ending brace found in parsing
void spec_brace()
{
        if(script_debug) C_Printf("brace\n");

        if(bracetype != bracket_close)  // only deal with closing } braces
                return;

                        // if() requires nothing to be done
        if(current_section->type == st_if) return;

        if(current_section->type == st_loop)
        {
                rover = current_section->data.data_loop.loopstart;
                return;
        }
}

        // 'if' statement
void spec_if()
{
        int endtoken;
        svalue_t eval;

        if( (endtoken = find_token(0, num_tokens-1, ")")) == -1)
        {
                script_error("parse error in if statement\n");
                return;
        }

                // 2 to skip past the 'if' and '('
        eval = evaluate_expression(2, endtoken-1);

        if(current_section && bracetype == bracket_open
           && endtoken == num_tokens-1)
        {
                current_section->type = st_if;  // mark as if

                // {} braces
                if(!intvalue(eval))       // skip to end of section
                        rover = current_section->end+1;
        }
        else    // if() without {} braces
                if(intvalue(eval))
                {
                                    // nothing to do ?
                        if(endtoken == num_tokens-1) return;
                        evaluate_expression(endtoken+1, num_tokens-1);
                }
}

void spec_while()      // while() loop
{
        int endtoken;
        svalue_t eval;

        if(!current_section)
        {
                script_error("no {} section given for loop\n");
                return;
        }

        if( (endtoken = find_token(0, num_tokens-1, ")")) == -1)
        {
                script_error("parse error in loop statement\n");
                return;
        }

        current_section->type = st_loop;    // mark as loop
        current_section->data.data_loop.loopstart = linestart;

        eval = evaluate_expression(2, endtoken-1);

                      // skip if no longer valid
        if(!intvalue(eval)) rover = current_section->end+1;
}

void spec_for()                 // for() loop
{
        svalue_t eval;
        int start;
        int comma1, comma2;     // token numbers of the seperating commas

        if(!current_section)
        {
                script_error("need {} delimiters for for()\n");
                return;
        }

        // is a valid section

        current_section->type = st_loop;
        current_section->data.data_loop.loopstart = linestart;

        start = 2;     // skip "for" and "(": start on third token(2)

        // find the seperating commas first

        if( (comma1 = find_token(start,    num_tokens-1, ",")) == -1
         || (comma2 = find_token(comma1+1, num_tokens-1, ",")) == -1)
        {
                script_error("incorrect arguments to if()\n");
                return;
        }

                     // are we looping back from a previous loop?
        if(current_section == prev_section)
        {
                // do the loop 'action' (third argument)
                evaluate_expression(comma2+1, num_tokens-2);

                // check if we should run the loop again (second argument)
                eval = evaluate_expression(comma1+1, comma2-1);
                if(!intvalue(eval))
                {
                        // stop looping
                        rover = current_section->end + 1;
                }
        }
        else
        {
                // first time: starting the loop
                // just evaluate the starting expression (first arg)
                evaluate_expression(start, comma1-1);
        }
}

/**************************** Variable Creation ****************************/

int newvar_type;

        // called for each individual variable in a statement
        //  newvar_type must be set
void create_variable(int start, int stop)
{
        if(killscript) return;

        if(tokentype[start] != name)
        {
                script_error("invalid name for variable: '%s'\n",
                                        tokens[start+1]);
                return;
        }

                    // check if already exists, only checking
                    // the current script
        if( variableforname(current_script, tokens[start]) )
                return;  // already one

        new_variable(current_script, tokens[start], newvar_type);

        if(stop != start) evaluate_expression(start, stop);
}

        // divide a statement (without type prefix) into individual
        // variables to be create them using create_variable
void parse_var_line(start)
{
        int starttoken = 1, endtoken;   // start on second token(1)
        
        while(1)
        {
                if(killscript) return;
                endtoken = find_token(starttoken, num_tokens-1, ",");
                if(endtoken == -1) break;
                create_variable(starttoken, endtoken-1);
                starttoken = endtoken+1;  //start next after end of this one
        }
                  // dont forget the last one
        create_variable(starttoken, num_tokens-1);
}

        // these functions merely set newvar_type, strip the prefix
        // from the statement and call parse_var_line

        // int %s.  Create a new integer variable
void spec_int()
{
        newvar_type = svt_int;

        parse_var_line();
}

        // string %s.  Create a new string variable
        // rather similar to spec_int =)
void spec_string()
{
        newvar_type = svt_string;

        parse_var_line();
}
