#ifndef __C_RUNCMD_H__
#define __C_RUNCMD_H__

typedef struct command_s command_t;
typedef struct variable_s variable_t;

#include "c_cmdlst.h"

/******************************** #defines ********************************/

#define MAXTOKENS 32
#define MAXTOKENLENGTH 64
#define CMDCHAINS 16

/********************************* ENUMS **********************************/

enum    // cmdsrc values
{
        c_typed,        // typed at console
        c_script,  // called by someone crossing a linedef
        c_netcmd,
        C_CMDTYPES
};

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
        cf_handlerset   =8,     // if set, the handler sets the variable,
                                // not c_runcmd.c itself
        cf_netvar       =16,    // sync with other pcs
        cf_level        =32,    // only works in levels
        cf_hidden       =64,    // hidden in cmdlist
};

enum    // variable type
{
        vt_int,                // normal integer 
        vt_float,              // decimal               NOT IMPLEMENTED
        vt_string,             // string
        vt_toggle              // on/off value          NOT IMPLEMENTED
};

/******************************** STRUCTS ********************************/

struct variable_s
{
                // NB: for strings, this points to a char*, not a char
        void *variable;
        void *v_default;         // the default 
        int type;       // vt_?? variable type: int, string
        int min;        // minimum value or string length
        int max;        // maximum value/length
        char **defines;  // strings representing the value: eg "on" not "1"
};

struct command_s
{
        char *name;
        int type;               // ct_?? command type
        int flags;              // cf_??
        variable_t *variable;
        void (*handler)();       // handler
        int netcmd;     // network command number
        command_t *next;        // for hashing
};

typedef struct
{
        char *name;
        char *command;
} alias_t;

/************************** PROTOTYPES/EXTERNS ****************************/

/***** command running ****/

extern int cmdtype;
extern char c_argv[MAXTOKENS][MAXTOKENLENGTH];
extern int c_argc;
extern char c_args[128];

void C_RunCommand(command_t *command, char *options);
void C_RunTextCmd(char *cmdname);

char *C_VariableValue(command_t *command);
char *C_VariableStringValue(command_t *command);

/**** tab completion ****/

void C_InitTab();
char *C_NextTab(char *key);
char *C_PrevTab(char *key);

/**** aliases ****/

extern alias_t aliases[128];
extern char *cmdoptions;

void C_Alias();
alias_t *C_NewAlias(unsigned char *aliasname, unsigned char *command);
void C_RemoveAlias(unsigned char *aliasname);
alias_t *C_GetAlias(char *name);

/**** command buffers ****/

void C_BufferCommand(int cmdtype, command_t *command,
                     char *options, int cmdsrc);
void C_RunBuffers();
void C_RunBuffer(int cmtype);
void C_BufferDelay(int, int);
void C_ClearBuffer(int);

/**** hashing ****/

extern command_t *cmdroots[CMDCHAINS];   // the commands in hash chains

void C_AddCommand(command_t *command);
void C_AddCommandList(command_t *list);
void C_AddCommands();
command_t *C_GetCmdForName(char *cmdname);

#endif
