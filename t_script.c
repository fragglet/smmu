/****************************** scripting **********************************/
                    // Copyright(c) 1999 Simon Howard //

// scripting.
//
// the current scripting system is a pretty temporary thing, this is
// only really here until i finish FraggleScript
//

#include "c_io.h"
#include "c_runcmd.h"
#include "p_mobj.h"
#include "t_script.h"
#include "w_wad.h"
#include "z_zone.h"

char *scripts[NUMSCRIPTS];       // the scripts
mobj_t *t_trigger;

//     T_ClearScripts()
//
//      called at level start, clears all scripts
//

void T_ClearScripts()
{
        int i;

        for(i=0; i<NUMSCRIPTS; i++)
        {
                if(scripts[i])
                        free(scripts[i]);
                scripts[i] = NULL;
        }
}

void T_RunScript(int n)
{
        char *rover, *startofline;

        if(n<0 || n>=NUMSCRIPTS) return;
        if(!scripts[n]) return;

        rover = startofline = scripts[n];

        while(*rover)
        {
                if(*rover == '\n')      // end of line
                {
                        *rover = 0;     // turn to end-of-string
                        cmdtype = c_script;
                        C_RunTextCmd(startofline);
                        *rover = '\n';  // back to end-of line
                        startofline = rover+1;
                }
                rover++;
        }
        cmdtype = c_script;
        C_RunTextCmd(startofline);      // run the last one
}

// console scripting debugging commands

        // console cmd: dump a script
void T_Dump()
{
        int scriptnum;

        if(!c_argc)
        {
                C_Printf("usage: T_DumpScript <scriptnum>\n");
                return;
        }

        scriptnum = atoi(c_argv[0]);

        if(!scripts[scriptnum])
        {
                C_Printf("script %i not defined.\n", scriptnum);
                return;
        }

        C_Printf(scripts[scriptnum]);
}

void T_ConsRun()
{
        int sn;

        if(!c_argc)
        {
                C_Printf("T_RunScript: run a script\n"
                         "usage: T_RunScript <script>\n");
                return;
        }

        sn = atoi(c_argv[0]);

        if(!scripts[sn])
        {
                C_Printf("script not defined\n");
        }
        T_RunScript(sn);
}
