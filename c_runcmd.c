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
int C_Strcmp(unsigned char *a, unsigned char *b);

/***************************
  PARSING/RUNNING COMMANDS
 ***************************/

int cmdtype;
char cmdtokens[MAXTOKENS][MAXTOKENLENGTH];
int numtokens;
char c_argv[MAXTOKENS][MAXTOKENLENGTH];   // tokenised list of arguments
int c_argc;                               // number of arguments
char c_args[128];                         // raw list of arguments

        // break up the command into tokens
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

        // C_RunIndivTextCmd.
        // called once a typed command line has been broken
        // into individual commands (multiple commands on one
        // line are allowed with the ';' character)

static void C_RunIndivTextCmd(char *cmdname)
{
        command_t *command;
        alias_t *alias;

                // cut off leading spaces
        while(*cmdname==' ') cmdname++;

                // break into tokens
        C_GetTokens(cmdname);

                // find the command being run from the first token.
        command = C_GetCmdForName(cmdtokens[0]);
        if(!numtokens) return; // no command
        if(!command)    // no _command_ called that
        {
                 // alias?
           if((alias = C_GetAlias(cmdtokens[0])))
           {
                // save the options into cmdoptions
              cmdoptions = cmdname + strlen(cmdtokens[0]);
              C_RunAlias(alias);
           }
           else         // no alias either
              C_Printf("unknown command: '%s'\n",cmdtokens[0]);
           return;
        }

                // run the command (buffer it)
        C_RunCommand(command, cmdname+strlen(cmdtokens[0]));
}

        // check the flags of a command to see if it
        // should be run or not

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

        // C_RunCommand.
        // call with the command to run and the command-line options.
        // buffers the commands which will be run later.

void C_RunCommand(command_t *command, char *options)
{
      // do not run straight away, we might be in the middle of rendering
        C_BufferCommand(cmdtype, command, options, cmdsrc);
}

        // actually run a command. Same as C_RunCommand only instant.

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
                if(command->handler)
                        command->handler();
                else
                        C_Printf("error: no command handler for %s\n", command->name);
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

        // see if the command needs to be sent to other computers
        // to maintain sync and do so if neccesary

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
                        // ignore ';'s inside quote marks
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

        // get the literal value of a variable (ie. "1" not "on")

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

        // get the string value (ie. "on" not "1")

char *C_VariableStringValue(command_t *command)
{
    static char value[128];

    if(!command->variable) return NULL;

                // does the variable have alternate 'defines' ?
    strcpy(value, command->variable->defines ?

              // print the 'define' (string representing the value)
    command->variable->defines[*(int*)command->variable->variable
                                -command->variable->min] :

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

        // is a string a number?
boolean isnum(char *text)
{
        for(;*text; text++)
                if((*text>'9' || *text<'0') && *text!='-') return false;
        return true;
}

        // take a string and see if it matches a define for a
        // variable. Replace with the literal value if so.

char* C_valuefordefine(variable_t *variable, char *s)
{
        int count;
        static char returnstr[10];

        if(variable->defines)
           for(count = variable->min;count <= variable->max; count++)
           {
              if(!C_Strcmp(s, variable->defines[count-variable->min]))
              {
                  sprintf(returnstr, "%i", count);
                  return returnstr;
              }
           }

        if(variable->type == vt_int && !isnum(s))
                return NULL;

        return s;
}
        // set a variable
void C_SetVariable(command_t *command)
{
        variable_t* variable;
        int size = 0;
        char *errormsg;
        char *temp;

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

        temp = C_valuefordefine(variable, c_argv[0]);

        if(temp)
                strcpy(c_argv[0], temp);
        else
        {
                C_Puts("not a possible value for '%s'", command->name);
                return;
        }      

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
                
                // netgame sync: send command to other nodes
        if(C_Sync(command)) return;

                   // now set it
                   // 5/8/99 set default value also
                   // 16/9/99 cf_handlerset flag for variables set from
                           // the handler instead
        if(!(command->flags & cf_handlerset))
           switch(variable->type)  // implicitly set the variable
           {
              case vt_int:
              *(int*)variable->variable = atoi(c_argv[0]);
              if(variable->v_default && cmdsrc==c_typed)  // default
                     *(int*)variable->v_default = atoi(c_argv[0]);
              break;
       
              case vt_string:
              free(*(char**)variable->variable);
              *(char**)variable->variable = strdup(c_argv[0]);
              if(variable->v_default && cmdsrc==c_typed)  // default
              {
                  free(*(char**)variable->v_default);
                  *(char**)variable->v_default = strdup(c_argv[0]);
              }
              break;
        
              default:
              return;
           }

        if(command->handler)          // run handler if there is one
                command->handler();
}

/**********************
        TAB COMPLETION
 **********************/

char origkey[100];
int nokey=1;
command_t *tabs[128];
int numtabs=0;
int thistab=-1;

        // given a key (eg. "r_sw"), will look through all
        // the commands in the hash chains and gather
        // all the commands which begin with this into a
        // list 'tabs'
void GetTabs(char *key)
{
        int i;
        int keylen;

        numtabs = 0;
        while(*key==' ') key++;

        strcpy(origkey, key);
        nokey = 0;

        if(!*key) return;

        keylen = strlen(key);

                // check each hash chain in turn

        for(i=0; i<CMDCHAINS; i++)
        {
           command_t* browser = cmdroots[i];
           for(; browser; browser = browser->next)
              if(!(browser->flags & cf_hidden) && // ignore hidden ones
                 !strncmp(browser->name, key, keylen))
              {
                 // found a new tab
                 tabs[numtabs] = browser;
                 numtabs++;
              }
        }
}

        // reset the tab list 
void C_InitTab()
{
        numtabs = 0;
        strcpy(origkey, "");
        nokey = 1;
        thistab = -1;
}

        // called when tab pressed. get the next tab
        // from the list
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

        // called when shift-tab pressed. get the
        // previous tab from the lift
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

/************************
                ALIASES
 ************************/

alias_t aliases[128];
char *cmdoptions;       // command line options for aliases

        // get an alias from a name
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

        return NULL;
}

        // create a new alias, or use one that already exists
alias_t *C_NewAlias(unsigned char *aliasname, unsigned char *command)
{
        alias_t *alias;

        alias=aliases;

        while(alias->name)
        {
                if(!strcmp(alias->name, c_argv[0]))
                {
                        free(alias->name);
                        free(alias->command);
                        break;
                }
                alias++;
        }

        alias->name = strdup(aliasname);
        alias->command = strdup(command);

        return alias;
}

       // remove an alias
void C_RemoveAlias(unsigned char *aliasname)
{
      alias_t *alias;

      alias = C_GetAlias(aliasname);
      if(!alias)
      {
            C_Printf("unknown alias \"%s\"\n", aliasname);
            return;
      }
      free(alias->name); free(alias->command);
      while(alias->name)
      {
            memcpy(alias, alias+1, sizeof(alias_t));
            alias++;
      }
}

        // console command to handle aliases
void C_Alias()
{
        alias_t *alias;
        char *temp;

        if(!c_argc)
        {
                // list em
                C_Printf(FC_GRAY"alias list:" FC_RED "\n\n");
                alias = aliases;
                while(alias->name)
                {
                        C_Printf("\"%s\": \"%s\"\n", alias->name,
					alias->command);
                        alias++;
                }
                if(alias==aliases) C_Printf("(empty)\n");
                return;
        }

        if(c_argc == 1)  // only one, remove alias
        {
                C_RemoveAlias(c_argv[0]);
                return;
        }

       // find it or make a new one

        temp = c_args + strlen(c_argv[0]);
        while(*temp == ' ') temp++;

        C_NewAlias(c_argv[0], temp);
}

        // run an alias
void C_RunAlias(alias_t *alias)
{
        while(*cmdoptions==' ') cmdoptions++;
        C_RunTextCmd(alias->command);   // run the command
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

        // compare regardless of font colour
int C_Strcmp(unsigned char *a, unsigned char *b)
{
        while(*a || *b)
        {
                   // remove colour dependency
                if(*a >= 128)   // skip colour
                {
                    a++; continue;
                }
                if(*b >= 128)
                {
                    b++; continue;
                }
                        // regardless of case also
                if(toupper(*a) != toupper(*b))
                        return 1;
                a++; b++;
        }

        return 0;       // no difference in them
}

/*************************
          COMMAND HASHING
 *************************/

command_t *cmdroots[16];

       // the hash key
#define CmdHashKey(s)                                     \
   (( (s)[0] + (s)[0] ? (s)[1] + (s)[1] ? (s)[2] +        \
                 (s)[2] ? (s)[3] + (s)[3] ? (s)[4]        \
                        : 0 : 0 : 0 : 0 ) % 16)

void C_AddCommand(command_t *command)
{
        int hash;

        hash = CmdHashKey(command->name);

        command->next = cmdroots[hash]; // hook it in at the start of
        cmdroots[hash] = command;       // the table

                // save the netcmd link
        if(command->flags & cf_netvar && command->netcmd==0)
                C_Printf("C_AddCommand: cf_netvar without a netcmd (%s)\n", command->name);

        c_netcmds[command->netcmd] = command;
}

        // add a list of commands terminated by one of type ct_end
void C_AddCommandList(command_t *list)
{
        for(;list->type != ct_end; list++)
                C_AddCommand(list);
}

extern void Cheat_AddCommands();
extern void     G_AddCommands();
extern void    HU_AddCommands();
extern void     R_AddCommands();
extern void     I_AddCommands();
extern void     S_AddCommands();
extern void   net_AddCommands();
extern void     V_AddCommands();

void C_AddCommands()
{
        C_AddCommandList(commands);

                // add commands in other modules
        Cheat_AddCommands();    // m_cheat.c
        G_AddCommands();        // g_cmd.c
        HU_AddCommands();       // hu_stuff.c
        R_AddCommands();        // r_main.c
        I_AddCommands();        // i_system.c
        S_AddCommands();        // s_sound.c
        net_AddCommands();      // d_net.c
        V_AddCommands();        // v_misc.c
}


        // get a command from a string if possible
        
command_t *C_GetCmdForName(char *cmdname)
{
        command_t *current;
        int hash;

        while(*cmdname==' ') cmdname++;

        strlwr(cmdname);

        // start hashing

        hash = CmdHashKey(cmdname);

        current = cmdroots[hash];
        while(current)
        {
                if(!strcmp(cmdname, current->name))
                        return current;
                current = current->next;        // try next in chain
        }

        return NULL;
}

        // console command to list commands
void C_Cmdlist()
{
        int numonline = 0;
        command_t *current;
        int i;
        int charnum;

                // list each command from the hash chains

                //  5/8/99 change: use hash table and 
                //  alphabetical order by first letter
        for(charnum=33; charnum < 'z'; charnum++)
          for(i=0; i<CMDCHAINS; i++)
            for(current = cmdroots[i]; current; current = current->next)
            {
              if(current->name[0]==charnum && !(current->flags & cf_hidden))
              {
                 C_Printf("%s ", current->name);
                 numonline++;
                 if(numonline >= 3)
                 {
                   numonline = 0;
                   C_Printf("\n");
                 }
               }
            }
        C_Printf("\n");
}


