#ifndef __VARIABLE_H__
#define __VARIABLE_H__

typedef struct svariable_s svariable_t;
#define VARIABLESLOTS 16

#include "t_parse.h"
#include "p_mobj.h"

struct svariable_s
{
        char *name;
        int type;       // vt_string or vt_int: same as in svalue_t
        union
        {
                char *s;
                long i;
                void (*handler)();      // for functions
                char *labelptr;         // for labels
                mobj_t *mobj;
        } value;
        svariable_t *next;       // for hashing
};

//extern svariable_t *svariables[MAXVARIABLES];

        // variables

void init_variables();
svariable_t *new_variable(script_t *script, char *name, int vtype);
svariable_t *find_variable(char *name);
svariable_t *variableforname(script_t *script, char *name);
svalue_t getvariablevalue(svariable_t *v);
void setvariablevalue(svariable_t *v, svalue_t newvalue);
void clear_variables(script_t *script);

        // functions

svalue_t evaluate_function(int start, int stop);   // actually run a function
svariable_t *new_function(char *name, void (*handler)() );

        // arguments to handler functions
#define MAXARGS 128
extern int t_argc;
extern svalue_t *t_argv;
extern svalue_t t_return;

#endif
