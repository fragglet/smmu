/***************************** FraggleScript ******************************/
                // Copyright(C) 1999 Simon Howard 'Fraggle' //
//
// Functions
//
// functions are stored as variables(see variable.c), the
// value being a pointer to a 'handler' function for the
// function. Arguments are stored in an argc/argv-style list
//
// this module contains all the handler functions for the
// basic FraggleScript Functions.

/* includes ************************/

#include <stdio.h>
#include "c_io.h"
#include "z_zone.h"
#include "doomstat.h"
#include "doomtype.h"
#include "hu_stuff.h"
#include "m_random.h"

#include "t_main.h"
#include "t_parse.h"
#include "t_spec.h"
#include "t_oper.h"
#include "t_vari.h"
#include "t_func.h"

svalue_t evaluate_expression(int start, int stop);
int find_token(int start, int stop, char *value);

        // functions. SF_ means Script Function not, well.. heh, me

        /////////// actually running a function /////////////


/*******************
  FUNCTIONS
 *******************/

// the actual handler functions for the
// functions themselves

// arguments are evaluated and passed to the
// handler functions using 't_argc' and 't_argv'
// in a similar way to the way C does with command
// line options.

// values can be returned from the functions using
// the variable 't_return'

void SF_Print()
{
        int i;

        if(!t_argc) return;

        for(i=0; i<t_argc; i++)
        {
                if(t_argv[i].type == svt_string)
                        C_Printf("%s", t_argv[i].value.s);
                else    // assume int
                        C_Printf("%i", (int)t_argv[i].value.i);
        }
}

        // return a random number from 0 to 255
void SF_Rnd()
{
        t_return.type = svt_int;
        t_return.value.i = P_Random(pr_script);
}

        // looping section. using the rover, find the highest level
        // loop we are currently in and return the section_t for it.

section_t *looping_section()
{
        section_t *best = NULL;         // highest level loop we're in
                                        // that has been found so far
        int n;

                // check thru all the hashchains
        for(n=0; n<SECTIONSLOTS; n++)
        {
           section_t *current = current_script->sections[n];

                // check all the sections in this hashchain
           while(current)
           {
                        // a loop?
                if(current->type == st_loop)
                       // check to see if it's a loop that we're inside
                   if(rover >= current->start && rover <= current->end)
                   {
                        // a higher nesting level than the best one so far?
                      if(!best || (current->start > best->start))
                          best = current;     // save it
                   }
                current = current->next;
           }
        }

        return best;    // return the best one found
}

        // "continue;" in FraggleScript is a function
void SF_Continue()
{
        section_t *section;

        if(!(section = looping_section()) )       // no loop found
        {
                script_error("continue() not in loop\n");
                return;
        }

        rover = section->end;      // jump to the closing brace
}

void SF_Break()
{
        section_t *section;

        if(!(section = looping_section()) )
        {
                script_error("break() not in loop\n");
                return;
        }
        rover = section->end+1;   // jump out of the loop
}

void SF_Goto()
{
        if(t_argc != 1)
        {
                script_error("incorrect arguments to goto\n");
                return;
        }

        // check argument is a labelptr

        if(t_argv[0].type != svt_label)
        {
                script_error("goto argument not a label\n");
                return;
        }

        // go there then if everythings fine

        rover = t_argv[0].value.labelptr;
}


void SF_Return()
{
        killscript = true;      // kill the script
}

void SF_Input()
{
        static char inputstr[128];

        gets(inputstr);

        t_return.type = svt_string;
        t_return.value.s = inputstr;
}

void SF_Beep()
{
        C_Printf("\a");
}

void SF_Clock()
{
        t_return.type = svt_int;
        t_return.value.i = (gametic*100)/35;
}

void SF_Centremsg()
{
       int i;
       char tempstr[128]="";

       for(i=0; i<t_argc; i++)
             if(t_argv[i].type == svt_string)
                    sprintf(tempstr,"%s%s", tempstr, t_argv[i].value.s);
             else    // assume int
                    sprintf(tempstr,"%s%i", tempstr, (int)t_argv[i].value.i);

       HU_centremsg(tempstr);
}

void SF_Playermsg()
{
       int i;
       char tempstr[128]="";

       for(i=0; i<t_argc; i++)
             if(t_argv[i].type == svt_string)
                    sprintf(tempstr,"%s%s", tempstr, t_argv[i].value.s);
             else    // assume int
                    sprintf(tempstr,"%s%i", tempstr, (int)t_argv[i].value.i);

       dprintf(tempstr);
}

extern void SF_StartScript();      // in t_script.c
extern void SF_Wait();

void init_functions()
{

                // add all the functions

                // important C-emulating stuff
        new_function("break", SF_Break);
        new_function("continue", SF_Continue);
        new_function("return", SF_Return);
        new_function("goto", SF_Goto);

                // standard FraggleScript functions
        new_function("print", SF_Print);
        new_function("rnd", SF_Rnd);
        new_function("input", SF_Input);
        new_function("beep", SF_Beep);
        new_function("clock", SF_Clock);
        new_function("wait", SF_Wait);
        new_function("startscript", SF_StartScript);

                // doom stuff
        new_function("centremsg", SF_Centremsg);
        new_function("playermsg", SF_Playermsg);
}
