// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//

#ifndef __PARSE_H__
#define __PARSE_H__

#include "p_mobj.h"     // for mobj_t

#define T_MAXTOKENS 64
#define TOKENLENGTH 128

        // furthered to include support for svt_mobj
#define intvalue(v)     \
        ( (v).type == svt_string ? atoi((v).value.s) :  \
          (v).type == svt_mobj ? -1 : (v).value.i )

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

  mobj_t *trigger;        // object which triggered this script
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

void run_script(script_t *script);
void continue_script(script_t *script, char *continue_point);
void parse_include(char *lumpname);
void run_statement();
void script_error(char *s, ...);

svalue_t evaluate_expression(int start, int stop);
int find_operator(int start, int stop, char *value);
int find_operator_backwards(int start, int stop, char *value);

/******* tokens **********/

typedef enum
{
  name,   // a name, eg 'count1' or 'frag'
  number,
  operator,
  string,
  unset,
  function          // function name
} tokentype_t;

enum    // brace types: where current_section is a { or }
{
  bracket_open,
  bracket_close
};

extern svalue_t nullvar;
extern int script_debug;

extern script_t *current_script;
extern mobj_t *trigger_obj;
extern int killscript;

extern char *tokens[T_MAXTOKENS];
extern tokentype_t tokentype[T_MAXTOKENS];
extern int num_tokens;
extern char *rover;     // current point reached in parsing
extern char *linestart; // start of the current expression

extern section_t *current_section;
extern section_t *prev_section;
extern int bracetype;

// the global_script is the root
// script and contains only built-in
// FraggleScript variables/functions

extern script_t global_script; 

#endif
