#ifndef __C_RUNCMD_H__
#define __C_RUNCMD_H__

#define MAXTOKENS 32
#define MAXTOKENLENGTH 64

// console command stuff

enum    // command type
{
        ct_command,
        ct_variable,
        ct_constant,
        ct_end
};
enum    // command flag
{
        cf_notnet       =1,     // not in netgames
        cf_netonly      =2,     // only in netgames
        cf_server       =4,     // server only 
        cf_hu_copy      =8,     // copy from the heads up text 
        cf_netvar       =16,    // sync with other pcs
        cf_level        =32     // only works in levels
};
enum    // variable type
{
        vt_int,                // normal integer 
        vt_float,              // decimal               NOT IMPLEMENTED
        vt_string,             // string
        vt_toggle              // on/off value          NOT IMPLEMENTED
};

typedef struct
{
                // NB: for strings, this points to a char*, not a char
        void *variable;
        int type;       // vt_?? variable type: int, string
        int min;        // minimum value or string length
        int max;        // maximum value/length
        char **defines;  // strings representing the value: eg "on" not "1"
} variable_t;

typedef struct
{
        char *name;
        int type;               // ct_?? command type
        int flags;              // cf_??
        variable_t *variable;
        void (*handler)();       // handler
        int netcmd;     // network command number
} command_t;


typedef struct
{
        char *name;
        char *command;
} alias_t;

        // individual commands, ie. with no ';' marks in
void C_RunCommand(command_t *command, char *options);
void C_RunTextCmd(char *cmdname);

command_t *C_GetCmdForName(char *cmdname);
char *C_VariableValue(command_t *command);
char *C_VariableStringValue(command_t *command);

void C_InitTab();
char *C_NextTab(char *key);
char *C_PrevTab(char *key);

alias_t *C_GetAlias(char *name);

void C_BufferCommand(int cmdtype, command_t *command, char *options,
                        int cmdsrc);
void C_RunBuffers();
void C_RunBuffer(int cmtype);
void C_BufferDelay(int, int);
void C_ClearBuffer(int);

extern alias_t aliases[128];
extern char *cmdoptions;

/*
                nb out of date!!! -- sf
                        FORMAT OF CONSOLE COMMANDS

 1. For each console command there is a entry in the 'commands' array
    in c_cmdlst.c
 
 2. Each command can be either ct_command, ct_variable or ct_constant
    (command, variable and constant value respectively)
 
 3. The way a command is handled depends on its type
        - ct_command: the function pointed to by command.handler is called
           with the options given on the command line
        - ct_constant: C_EchoValue is called which displays the value
           of the variable pointed to by command.variable->variable
        - ct_variable: The most complex:
           if(command line empty) C_SetValue is called which displays the
                        value of command.variable->variable
           else           
           if(new value within limits of command.variable->min
                        and command.variable->max)
           if(command.handler) command.handler(command line options)
           else command.variable->variable = new value (command line)
 
 4. A command can have a number of 'flags' (command.flags)
        - cf_notnet: will not work in netgames
        - cf_netonly: only works in netgames
        - cf_server: in netgames, can only be changed or run by the server
        - cf_hu_copy: after being run, players[consoleplayer].message is
                        written to the console
        - cf_netvar: when set, this command, when called, will be run on
                        all pcs in the netgame. How it does this:
          a) the command number is found (entry in the 'commands' cmdlst)
          b) the command number is put into a string, form:
                        "&%x %s",cmdnum, cmdline_options
          c) this string is broadcasted across the network to all pcs using
                C_SendCmd(c_net.c)
          d) the other pcs get the command, convert the hex number back into
             the command number and run the command
             NB. If this command is set, there will be a delay between the
                 command being run and it being executed
        - cf_level: will not be run unless gamestate==gs_level
5. Commands beginning with a '_' are special network commands and are
   ignored by the console 'cmdlist' command. Commands should not begin
   with a '&' (cf_netvar)

*/

enum    // for below
{
        c_typed,        // typed at console
        c_script,  // called by someone crossing a linedef
        c_netcmd,
        C_CMDTYPES
};

extern int cmdtype;
extern char c_argv[MAXTOKENS][MAXTOKENLENGTH];
extern int c_argc;
extern char c_args[128];
#endif
