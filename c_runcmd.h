#ifndef __C_RUNCMD_H__
#define __C_RUNCMD_H__

typedef struct command_s command_t;
typedef struct variable_s variable_t;

/******************************** #defines ********************************/

#define MAXTOKENS 32
#define MAXTOKENLENGTH 64
#define CMDCHAINS 16

// zdoom _inspired_:
#define CONSOLE_COMMAND(name, flags)                    \
        void Handler_ ## name();                        \
        command_t Cmd_ ## name = { # name, ct_command,  \
                       flags, NULL, Handler_ ## name,   \
                       0 };                             \
        void Handler_ ## name()

        // variable
#define CONSOLE_VARIABLE(name, variable, flags)                         \
        void Handler_ ## name();                                        \
        command_t Cmd_ ## name = { # name, ct_variable,                 \
                        flags, &var_ ## variable, Handler_ ## name,     \
                        0 };                                            \
        void Handler_ ## name()

#define CONSOLE_NETCMD(name, flags, netcmd)             \
        void Handler_ ## name();                        \
        command_t Cmd_ ## name = { # name, ct_command,  \
                       (flags) | cf_netvar, NULL,       \
                       Handler_ ## name, netcmd };      \
        void Handler_ ## name()

#define CONSOLE_NETVAR(name, variable, flags, netcmd)                   \
        void Handler_ ## name();                                        \
        command_t Cmd_ ## name = { # name, ct_variable,                 \
                        cf_netvar | (flags), &var_ ## variable,         \
                        Handler_ ## name, netcmd };                     \
        void Handler_ ## name()

#define CONSOLE_CONST(name, variable)                           \
        command_t Cmd_ ## name = { # name, ct_constant, 0,      \
                &var_ ## variable, NULL, 0 };           

#define VARIABLE(name, defaultvar, type, min, max, strings)  \
        variable_t var_ ## name = { &name, defaultvar,       \
                        type, min, max, strings};

#define VARIABLE_INT(name, defaultvar, min, max, strings)    \
        variable_t var_ ## name = { &name, defaultvar,       \
                        vt_int, min, max, strings};

#define VARIABLE_STRING(name, defaultvar, max)               \
        variable_t var_ ## name = { &name, defaultvar,       \
                        vt_string, 0, max, NULL};

#define VARIABLE_BOOLEAN(name, defaultvar, strings)          \
        variable_t var_ ## name = { &name, defaultvar,       \
                        vt_int, 0, 1, strings };

#define CONST_INT(name)                                      \
        variable_t var_ ## name = { &name, NULL,             \
                        vt_int, -1, -1, NULL};

#define CONST_STRING(name)                                   \
        variable_t var_ ## name = { &name, NULL,             \
                        vt_string, -1, -1, NULL};


#define C_AddCommand(c)  (C_AddCommand)(&Cmd_ ## c) 

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

void (C_AddCommand)(command_t *command);
void C_AddCommandList(command_t *list);
void C_AddCommands();
command_t *C_GetCmdForName(char *cmdname);

/***** define strings for variables *****/

extern char *yesno[];
extern char *onoff[];
extern char *colournames[];
extern char *textcolours[];
extern char *skills[];

#endif
