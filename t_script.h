// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//

#ifndef __T_SCRIPT_H__
#define __T_SCRIPT_H__

typedef struct runningscript_s runningscript_t;

#include "p_mobj.h"
#include "t_parse.h"

struct runningscript_s
{
  script_t *script;
  
  // where we are
  char *savepoint;
  int timer;      // delay time remaining
	
  // saved variables
  svariable_t *variables[VARIABLESLOTS];
  
  runningscript_t *prev, *next;  // for chain
  mobj_t *trigger;
};

void T_Init();
void T_ClearScripts();
void T_RunScript(int n);
void T_PreprocessScripts();
void T_DelayedScripts();
mobj_t *MobjForSvalue(svalue_t svalue);

        // console commands
void T_Dump();
void T_ConsRun();

#define MAXSCRIPTS 128

extern script_t levelscript;
extern script_t *scripts[MAXSCRIPTS];       // the scripts
extern mobj_t *t_trigger;

#endif
