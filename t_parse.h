#ifndef __PARSE_H__
#define __PARSE_H__

#include "p_mobj.h"     // for mobj_t

#define T_MAXTOKENS 64
#define TOKENLENGTH 128

#define intvalue(v)     \
        ( (v).type == svt_string ? atoi((v).value.s) : (v).value.i )

typedef struct script_s script_t;
typedef struct svalue_s svalue_t;
typedef struct operator_s operator_t;

struct svalue_s
{
        int type;
        union
        {
                long i;
                char *s;
                char *labelptr; // goto() label
                mobj_t *mobj;
        } value;
};

#include "t_vari.h"
#include "t_prepro.h"

struct script_s
{
        // script data

        char *data;
        int scriptnum;  // this script's number
        int len;

        // {} sections

        section_t *sections[SECTIONSLOTS];

        // variables:

        svariable_t *variables[VARIABLESLOTS];

                // ptr to the parent script
                // the parent script is the script above this level
                // eg. individual linetrigger scripts are children
                // of the levelscript, which is a child of the
                // global_script
        script_t *parent;
};

struct operator_s
{
        char *string;
        svalue_t (*handler)(int, int, int); // left, mid, right
        int direction;
};

enum
{
        forward,
        backward
};

enum
{
        svt_string,
        svt_int,
        svt_function,     // functions are stored as variables
        svt_label,        // labels for goto calls are variables
};

svalue_t evaluate_expression(int start, int stop);
void run_statement();
char *run_line(char *data);
void run_script(script_t *script);
void continue_script(script_t *script, char *continue_point);
int find_token(int start, int stop, char *value);
void script_error(char *s, ...);

/******* tokens **********/

typedef enum {
        name,   // a name, eg 'count1' or 'frag'
        number,
        operator,
        string,
        unset,
        bracket_open,
        bracket_close,
        function          // function name
} tokentype_t;

extern char *tokens[T_MAXTOKENS];
extern tokentype_t tokentype[T_MAXTOKENS];
extern int num_tokens;
extern script_t *current_script;
extern svalue_t nullvar;
extern int script_debug;
extern char *rover;     // current point reached in parsing
extern char *linestart; // start of the current expression
extern int killscript;

extern section_t *current_section;
extern section_t *prev_section;
extern int bracetype;

extern script_t global_script;  // the global_script is the root
                        // script and contains only built-in
                        // FraggleScript variables/functions

#endif
