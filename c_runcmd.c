/******************************* console **********************************/
                // Copyright(C) 1999 Simon Howard 'Fraggle' //
//
// Command running functions
//
// Running typed commands, or network/linedef triggers. Sending commands over
// the network. Calls handlers for some commands in c_handle.c
//


/* includes ************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "c_io.h"
#include "c_runcmd.h"
#include "c_cmdlst.h"
#include "c_net.h"

#include "doomdef.h"
#include "doomstat.h"
#include "t_script.h"
#include "g_game.h"
#include "z_zone.h"

/* Prototypes **********************/

void C_EchoValue(command_t *command);
void C_SetVariable(command_t *command);
void C_RunAlias(alias_t *alias);
int C_Sync(command_t *command);
void C_ArgvtoArgs();

/* Variables ***********************/

alias_t aliases[128];
char *cmdoptions;       // command line options for aliases

int cmdtype;
char cmdtokens[MAXTOKENS][MAXTOKENLENGTH];
int numtokens;
char c_argv[MAXTOKENS][MAXTOKENLENGTH];   // tokenised list of arguments
int c_argc;                               // number of arguments
char c_args[128];                         // raw list of arguments

/* Functions ***********************/

void C_GetTokens(char *command)
{
        char *rover;
        boolean quotemark=0;

        rover = command;
        cmdtokens[0][0] = 0;
        numtokens = 1;

        while(*rover == ' ') rover++;
        if(!*rover)     // end of string already
        {
                numtokens = 0;
                return;
        }

        while(*rover)
        {
                if(*rover=='"')
                {
                        quotemark = !quotemark;
                        rover++;
                        continue;
                }
                if(*rover==' ' && !quotemark)   // end of token
                {
                // only if the current one actually contains something
                   if(cmdtokens[numtokens-1][0])
                   {
                       numtokens++;
                       cmdtokens[numtokens-1][0] = 0;
                   }
                   rover++;
                   continue;
                }
                        // add char to line
                sprintf(cmdtokens[numtokens-1], "%s%c",
                        cmdtokens[numtokens-1], *rover);
                rover++;
        }
}

static void C_RunIndivTextCmd(char *cmdname)
{
        command_t *command;
        alias_t *alias;

        while(*cmdname==' ') cmdname++;

        C_GetTokens(cmdname);

        command = C_GetCmdForName(cmdtokens[0]);
        if(!numtokens) return; // no command
        if(!command)    // no _command_ called that
        {
                 // alias?
           if( (alias = C_GetAlias(cmdtokens[0])) )  // ()s: shut up compiler
           {
              cmdoptions = cmdname + strlen(cmdtokens[0]);
              C_RunAlias(alias);
           }
           else
              C_Printf("unknown command: '%s'\n",cmdtokens[0]);
           return;
        }
        C_RunCommand(command, cmdname+strlen(cmdtokens[0]));
}

boolean C_CheckFlags(command_t *command)
{
        char *errormsg;
                        // check the flags
        errormsg = NULL;

        if((command->flags & cf_notnet) && (netgame && !demoplayback))
                errormsg = "command not available in netgame";
        if((command->flags & cf_netonly) && !netgame && !demoplayback)
                errormsg = "command only available in netgame";
        if((command->flags & cf_server) && consoleplayer && !demoplayback
                && cmdtype!=c_netcmd)
                errormsg = "command for server only";
        if((command->flags & cf_level) && gamestate!=GS_LEVEL)
                errormsg = "command can be run in levels only";

        if(errormsg)
        {
                C_Puts(errormsg);
                return true;
        }

        return false;
}

void C_RunCommand(command_t *command, char *options)
{
      // do not run straight away, we might be in the middle of rendering
        C_BufferCommand(cmdtype, command, options, cmdsrc);
}

void C_DoRunCommand(command_t *command, char *options)
{
        int i;

        C_GetTokens(options);

        memcpy(c_argv, cmdtokens, sizeof cmdtokens);
        c_argc = numtokens;

        // perform checks

                // check through the tokens for variable names
        for(i=0 ; i<c_argc ; i++)
        {
            if(c_argv[i][0]=='%' || c_argv[i][0]=='$') // variable
            {
                command_t *variable;

                variable = C_GetCmdForName(c_argv[i]+1);
                if(!variable || !variable->variable)
                {
                     C_Printf("unknown variable '%s'\n",c_argv[i]+1);
                          // clear for next time
                     cmdtype = c_typed; cmdsrc = consoleplayer;
                     return;
                }

                strcpy(c_argv[i],
                        c_argv[i][0]=='%' ? C_VariableValue(variable) :
                                C_VariableStringValue(variable) );
            }
        }

        C_ArgvtoArgs();                 // build c_args
                       // actually do this command
        switch(command->type)
        {
                case ct_command:
                               // not to be run ?
                if(C_CheckFlags(command) ||
                   C_Sync(command))
                {
                   cmdtype = c_typed; cmdsrc = consoleplayer; 
                   return;
                }
                command->handler();
                break;

                case ct_constant:
                C_EchoValue(command);
                break;

                case ct_variable:
                C_SetVariable(command);
                break;

                default:
                C_Printf("unknown command type %i\n",command->type);
                break;
        }

        cmdtype = c_typed; cmdsrc = consoleplayer;   // clear for next time
}

// take all the argvs and put them all together in the args string

void C_ArgvtoArgs()
{
        int i, n;

        for(i=0; i<c_argc; i++)
        {
                if(!c_argv[i][0])       // empty string
                {
                  for(n=i; n<c_argc-1; n++)
                        strcpy(c_argv[n],c_argv[n+1]);
                  c_argc--; i--;
                }
        }

        c_args[0] = 0;

        for(i=0 ; i<c_argc; i++)
                sprintf(c_args, "%s%s ", c_args, c_argv[i]);
}

// return a string of all the argvs linked together, but with each
// argv in quote marks "

char *C_QuotedArgvToArgs()
{
        int i;
        static char returnvar[1024];

        returnvar[0] = 0;

        for(i=0 ; i<c_argc; i++)
                sprintf(returnvar, "%s\"%s\" ", returnvar, c_argv[i]);

        return returnvar;
}


int C_Sync(command_t *command)
{
        if(command->flags & cf_netvar)
        {
           if(cmdtype != c_netcmd)      // dont get stuck repeatedly
                                        // sending the same command
           {                               // send to sync
              C_SendCmd(CN_BROADCAST, command->netcmd,
                        "%s", C_QuotedArgvToArgs());
              return true;
           }
        }
        return false;
}

        // execute a compound command (with or without ;'s)
void C_RunTextCmd(char *command)
{
        char *startofcmd;
        char *scanner;
        int quotemark=0;  // for " quote marks

        startofcmd = scanner = command;

                // break down the command into individual commands
        while(*scanner)
        {
                if(*scanner=='"') quotemark = !quotemark;
                if(*scanner==';' && !quotemark)
                {                      // end of command: run it
                        *scanner = '\0';
                        C_RunIndivTextCmd(startofcmd);
                        *scanner = ';';
                        startofcmd=scanner+1; // prepare for next command
                }
                scanner++;
        }
        C_RunIndivTextCmd(startofcmd); // dont forget the last one
}

char *C_VariableValue(command_t *command)
{
    static char value[128];

    if(!command->variable) return NULL;

    switch(command->variable->type)
    {
          case vt_int:
          case vt_toggle:
          sprintf(value, "%i", *(int*)command->variable->variable);
          break;

          case vt_string:
          sprintf(value, "%s", *(char**)command->variable->variable);
          break;

          default:
          sprintf(value, "(unknown)");
          break;
    }

    return value;
}

char *C_VariableStringValue(command_t *command)
{
    static char value[128];

    if(!command->variable) return NULL;

    strcpy(value, command->variable->defines ?

              // print the 'define' (string representing the value)
    command->variable->defines[*(int*)command->variable->variable] :

              // otherwise print the literal value
        C_VariableValue(command) );

    return value;
}


        // echo a value eg. ' "alwaysmlook" is "1" '
void C_EchoValue(command_t *command)
{
    C_Printf("\"%s\" is \"%s\"\n", command->name,
        C_VariableStringValue(command) );
}

char* C_valuefordefine(variable_t *variable, char *s)
{
        int count;
        static char returnstr[100];

        if(variable->type == vt_string) return s;

        if(variable->defines)
                for(count = 0;count <= variable->max; count++)
                {
                        if(!stricmp(s, variable->defines[count]))
                        {
                                sprintf(returnstr, "%i", count);
                                return returnstr;
                        }
                }

        return s;
}
        // set a variable
void C_SetVariable(command_t *command)
{
        variable_t* variable;
        int size = 0;
        char *errormsg;

        // cut off the leading spaces

        if(!c_argc)     // asking for value
        {
                C_EchoValue(command);
                return;
        }

        // change it?
        if(C_CheckFlags(command)) return;       // no

                // ok, set the value
        variable = command->variable;

        strcpy(c_argv[0], C_valuefordefine(variable, c_argv[0]));

        switch(variable->type)
        {
                case vt_int:
                size = atoi(c_argv[0]);
                break;

                case vt_string:
                size = strlen(c_argv[0]);
                break;

                default:
                return;
        }

        // check the min/max sizes

        errormsg = NULL;
        if(size > variable->max)
                errormsg = "value too big";
        if(size < variable->min)
                errormsg = "value too small";
        if(errormsg)
        {
                C_Puts(errormsg); return;
        }

        if(C_Sync(command)) return;

                   // now set it
        if(command->handler)          // use handler if there is one
        {
                command->handler();
        }
        else
                switch(variable->type)  // implicitly set the variable
                {
                        case vt_int:
                        *(int*)variable->variable = atoi(c_argv[0]);
                        break;
        
                        case vt_string:
                        free(*(char**)variable->variable);
                        *(char**)variable = strdup(c_argv[0]);
                        break;
        
                        default:
                        return;
                }

}

// tab completion

char origkey[100];
int nokey=1;
command_t *tabs[128];
int numtabs=0;
int thistab=-1;

void GetTabs(char *key)
{
        command_t* browser=commands;

        numtabs=0;
        while(*key==' ') key++;

        strcpy(origkey,key);
        nokey=0;

        if(!*key) return;

        while(browser->type!=ct_end)
        {
                if(!strncmp(key,browser->name,strlen(key)))
                {
                        tabs[numtabs]=browser;
                        numtabs++;
                }
                browser++;
        }
}

void C_InitTab()
{
        numtabs=0;
        strcpy(origkey,"");
        nokey=1;
        thistab=-1;
}

char *C_NextTab(char *key)
{
        static char returnstr[100];
        if(nokey)
        {
                GetTabs(key);
        }

        if(thistab==-1) thistab=0;
        else thistab++;

        if(thistab>=numtabs)
        {
                thistab=-1;
                return origkey;
        }

        sprintf(returnstr,"%s ",tabs[thistab]->name);
        return returnstr;
}

char *C_PrevTab(char *key)
{
        static char returnstr[100];

        if(nokey)
        {
                GetTabs(key);
        }

        if(thistab==-1) thistab=numtabs-1;
        else thistab--;

        if(thistab<0)
        {
                thistab=-1;
                return origkey;
        }

        sprintf(returnstr,"%s ",tabs[thistab]->name);
        return returnstr;
}

// end tab completion

alias_t *C_GetAlias(char *name)
{
        alias_t *alias=aliases;

        while(alias->name)
        {
                if(!strcmp(name,alias->name))
                {
                        return alias;
                }
                alias++;
        }

        return 0;
}

// i might want to replace this with a hash lookup table for speed if I
// find that I'm running lots of console commands (scripts. key bindings.
// net commands..)

command_t *C_GetCmdForName(char *cmdname)
{
        command_t *current;

        while(*cmdname==' ') cmdname++;

        strlwr(cmdname);

        current=commands;

        while(current->type != ct_end)
        {
                if(!strcmp(current->name,cmdname)) // found the command
                {
                        break;
                }
                current++;
        }

        if(current->type == ct_end) return NULL;

        return current;
}

void C_RunAlias(alias_t *alias)
{
        while(*cmdoptions==' ') cmdoptions++;
        C_RunTextCmd(alias->command);  // just run the command at the moment
}

/*********************
  COMMAND BUFFERING
 *********************/

// new ticcmds can be built at any time including during the
// rendering process. The commands need to be buffered
// and run by the tickers instead of directly from the
// responders
//
// a seperate buffered command list is kept for each type
// of command (typed, script, etc)

#define MAXBUFFERED 128

typedef struct
{
        command_t *command;     // the command
        char *options;          // command line options
        int cmdsrc;             // source player
        boolean run;             // run yet? kind of a hack
} bufferedcmd;

typedef struct
{
        bufferedcmd cmdbuffer[MAXBUFFERED];
        int cmds;       // number of buffered commands
        int timer;      // tic timer to temporarily freeze executing of cmds
} cmdbuffer;

cmdbuffer buffers[C_CMDTYPES];

        // for simplicity:
#define bufcmd (buffers[cmdtype].cmdbuffer)

void C_BufferCommand(int cmdtype, command_t *command, char *options,
                        int cmdsrc)
{
                // add to appropriate list
        bufcmd [buffers[cmdtype].cmds].command = command;
        bufcmd [buffers[cmdtype].cmds].options = strdup(options);
        bufcmd [buffers[cmdtype].cmds].cmdsrc = cmdsrc;
        bufcmd [buffers[cmdtype].cmds].run = false;
        buffers [cmdtype].cmds ++;
}

void C_RunBuffer(int cmtype)
{
        int i;

        if(buffers[cmtype].timer)      // buffer frozen
        {
                  buffers[cmtype].timer--;
                  return;
        }

        for(i=0; i<buffers[cmtype].cmds; i++)
        {
             if(buffers[cmtype].cmdbuffer[i].run) continue; // already run

                // need to check twice as a countdown may have
                // been started by one of the commands in this buffer

             if(buffers[cmtype].timer)      // countdown ticker
             {
                  buffers[cmtype].timer--;
                  return;
             }
             else
             {
                  cmdtype = cmtype;
                  cmdsrc = buffers[cmtype].cmdbuffer[i].cmdsrc;
                  t_trigger = players[cmdsrc].mo;
                  C_DoRunCommand(buffers[cmtype].cmdbuffer[i].command,
                            buffers[cmtype].cmdbuffer[i].options);
                  buffers[cmtype].cmdbuffer[i].run = true;
             }
        }
        buffers[cmtype].cmds = 0;        // empty it
}

void C_RunBuffers()
{
        int i;

        for(i=0; i<C_CMDTYPES; i++)
                C_RunBuffer(i);
}

void C_BufferDelay(int cmdtype, int delay)
{
        buffers[cmdtype].timer += delay;
}

void C_ClearBuffer(int cmdtype)
{
        buffers[cmdtype].timer = 0;     // stop timer
        buffers[cmdtype].cmds = 0;      // empty 
}


