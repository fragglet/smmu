#ifndef __T_SCRIPT_H__
#define __T_SCRIPT_H__

#include "p_mobj.h"

void T_ClearScripts();
void T_RunScript(int n);

        // console commands
void T_Dump();
void T_ConsRun();

#define NUMSCRIPTS 128

extern char *scripts[NUMSCRIPTS];       // the scripts
extern mobj_t *t_trigger;

#endif
