// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// scripting.
//
// delayed scripts, running scripts, console cmds etc in here
// the interface between FraggleScript and the rest of the game
//
// By Simon Hoawrd
//
//----------------------------------------------------------------------------

#include "doomstat.h"
#include "c_io.h"
#include "c_net.h"
#include "c_runcmd.h"
#include "d_net.h"
#include "p_info.h"
#include "p_mobj.h"
#include "w_wad.h"
#include "z_zone.h"

#include "t_script.h"
#include "t_parse.h"
#include "t_vari.h"
#include "t_func.h"


void clear_runningscripts();

// the level script is just the stuff put in the wad,
// which the other scripts are derivatives of
script_t levelscript;

// the individual scripts
script_t *scripts[MAXSCRIPTS];       // the scripts
mobj_t *t_trigger;

runningscript_t runningscripts;        // first in chain

//     T_Init()
//
//    called at program start

void T_Init()
{
  init_variables();
  init_functions();
}

//     T_ClearScripts()
//
//      called at level start, clears all scripts
//

void T_ClearScripts()
{
  int i;
  
  // clear the indiv. scripts themselves
  
  for(i=0; i<MAXSCRIPTS; i++)
    scripts[i] = NULL;
  
  // stop runningscripts
  clear_runningscripts();
  
  // clear the levelscript
  levelscript.data = Z_Malloc(5, PU_LEVEL, 0);  // empty data
  levelscript.data[0] = NULL;
  
  levelscript.scriptnum = -1;
  levelscript.parent = &global_script;

  // clear levelscript variables
  
  for(i=0; i<VARIABLESLOTS; i++)
    {
      levelscript.variables[i] = NULL;
    }
}

void T_PreprocessScripts()
{
  // run the levelscript first
  // get the other scripts
  
  // levelscript started by player 0 'superplayer'
  levelscript.trigger = players[0].mo;
  
  preprocess(&levelscript);
  run_script(&levelscript);
}

void T_RunScript(int n)
{
  if(n<0 || n>=MAXSCRIPTS) return;
  if(!scripts[n]) return;
  
  scripts[n]->trigger = t_trigger;        // save trigger in script
  
  run_script(scripts[n]);
}

// console scripting debugging commands

CONSOLE_COMMAND(t_dump, 0)
{
  script_t *script;
  
  if(!c_argc)
    {
      C_Printf("usage: T_DumpScript <scriptnum>\n");
      return;
    }
  
  script = !strcmp(c_argv[0], "global") ? &levelscript :
    scripts[atoi(c_argv[0])];
  
  if(!script)
    {
      C_Printf("script '%s' not defined.\n", c_argv[0]);
      return;
    }
  
  C_Printf("%s\n", script->data);
}

CONSOLE_COMMAND(t_run, cf_level)
{
  int sn;
  
  if(!c_argc)
    {
      C_Printf("T_RunScript: run a script\n"
	       "usage: T_RunScript <script>\n");
      return;
    }
  
  sn = atoi(c_argv[0]);
  
  if(!scripts[sn])
    {
      C_Printf("script not defined\n");
      return;
    }
  t_trigger = players[cmdsrc].mo;
  
  T_RunScript(sn);
}

/************************
         PAUSING SCRIPTS
 ************************/

runningscript_t *freelist=NULL;      // maintain a freelist for speed

runningscript_t *new_runningscript()
{
  // check the freelist
  if(freelist)
    {
      runningscript_t *returnv=freelist;
      freelist = freelist->next;
      return returnv;
    }
  
  // alloc static: can be used in other levels too
  return Z_Malloc(sizeof(runningscript_t), PU_STATIC, 0);
}

static void free_runningscript(runningscript_t *runscr)
{
  // add to freelist
  runscr->next = freelist;
  freelist = runscr;
}

void T_DelayedScripts()
{
  runningscript_t *current, *next;
  int i;

  if(!info_scripts) return;       // no level scripts
  
  current = runningscripts.next;
  
  while(current)
    {
      if(--current->timer <= 0)
	{
	  // copy out the script variables from the
	  // runningscript_t

	  for(i=0; i<VARIABLESLOTS; i++)
	    current->script->variables[i] = current->variables[i];
	  current->script->trigger = current->trigger; // copy trigger
	  
	  // continue the script

	  continue_script(current->script, current->savepoint);
	  
	  // unhook from chain and free

	  current->prev->next = current->next;
	  if(current->next) current->next->prev = current->prev;
	  next = current->next;   // save before freeing
	  free_runningscript(current);
	}
      else
	next = current->next;
      current = next;   // continue to next in chain
    }
                
}

// script function

void SF_Wait()
{
  runningscript_t *runscr;
  int i;

  if(t_argc != 1)
    {
      script_error("incorrect arguments to function\n");
      return;
    }

  runscr = new_runningscript();
  runscr->script = current_script;
  runscr->savepoint = rover;
  runscr->timer = (intvalue(t_argv[0]) * 35) / 100;

  // hook into chain at start
  
  runscr->next = runningscripts.next;
  runscr->prev = &runningscripts;
  runscr->prev->next = runscr;
  if(runscr->next)
    runscr->next->prev = runscr;
  
  // save the script variables 
  for(i=0; i<VARIABLESLOTS; i++)
    {
      runscr->variables[i] = current_script->variables[i];
      
      // remove all the variables from the script variable list
      // to prevent them being removed when the script stops

      while(current_script->variables[i] &&
	    current_script->variables[i]->type != svt_label)
	current_script->variables[i] =
	  current_script->variables[i]->next;
    }
  runscr->trigger = current_script->trigger;      // save trigger
  
  killscript = true;      // stop the script
}

extern mobj_t *trigger_obj;           // in t_func.c

void SF_StartScript()
{
  runningscript_t *runscr;
  int i, snum;
  
  if(t_argc != 1)
    {
      script_error("incorrect arguments to function\n");
      return;
    }
  
  snum = intvalue(t_argv[0]);
  
  if(!scripts[snum])
    {
      script_error("script %i not defined\n", snum);
    }
  
  runscr = new_runningscript();
  runscr->script = scripts[snum];
  runscr->savepoint = scripts[snum]->data; // start at beginning
  runscr->timer = 1;      // start straight away

  // hook into chain at start
  
  runscr->next = runningscripts.next;
  runscr->prev = &runningscripts;
  runscr->prev->next = runscr;
  if(runscr->next)
    runscr->next->prev = runscr;
  
  // save the script variables 
  for(i=0; i<VARIABLESLOTS; i++)
    {
      runscr->variables[i] = scripts[snum]->variables[i];
      
      // in case we are starting another current_script:
      // remove all the variables from the script variable list
      // we only start with the basic labels
      while(runscr->variables[i] &&
	    runscr->variables[i]->type != svt_label)
	runscr->variables[i] =
	  runscr->variables[i]->next;
    }
  // copy trigger
  runscr->trigger = current_script->trigger;

}

// running scripts

CONSOLE_COMMAND(t_running, 0)
{
  runningscript_t *current;
  
  current = runningscripts.next;
  
  C_Printf(FC_GRAY "running scripts\n" FC_RED);
  
  if(!current)
    C_Printf("no running scripts.\n");
  
  while(current)
    {
      C_Printf("%i: T-%i tics\n", current->script->scriptnum,
	       current->timer);
      current = current->next;
    }
}

void clear_runningscripts()
{
  runningscript_t *runscr, *next;
  
  runscr = runningscripts.next;
  
  // free the whole chain
  while(runscr)
    {
      next = runscr->next;
      free_runningscript(runscr);
      runscr = next;
    }
  runningscripts.next = NULL;
}

mobj_t *MobjForSvalue(svalue_t svalue)
{
  int intval ;
  
  if(svalue.type == svt_mobj)
    return svalue.value.mobj;
  
  // this requires some creativity. We use the intvalue
  // as the thing number of a thing in the level.
  
  intval = intvalue(svalue);        
  
  if(intval < 0 || intval >= numthings)
    { script_error("no levelthing %i\n", intval); return NULL;}
  
  return spawnedthings[intval];
}


/*********************
            ADD SCRIPT
 *********************/

// when the level is first loaded, all the
// scripts are simply stored in the levelscript.
// before the level starts, this script is
// preprocessed and run like any other. This allows
// the individual scripts to be derived from the
// levelscript. When the interpreter detects the
// 'script' keyword this function is called

void spec_script()
{
  int scriptnum;
  int datasize;
  
  if(!current_section)
    {
      script_error("need seperators for script\n");
      return;
    }
  
  // presume that the first token is "script"
  
  if(num_tokens < 2)
    {
      script_error("need script number\n");
      return;
    }

  scriptnum = intvalue(evaluate_expression(1, num_tokens-1));
  
  if(scriptnum < 0)
    {
      script_error("invalid script number\n");
      return;
    }
  
  scripts[scriptnum] = Z_Malloc(sizeof(script_t), PU_LEVEL, 0);
  
  // copy script data
  // workout script size: -2 to ignore { and }
  datasize = current_section->end - current_section->start - 2;

  // alloc extra 10 for safety
  scripts[scriptnum]->data = Z_Malloc(datasize+10, PU_LEVEL, 0);
 
  // copy from parent script (levelscript) 
  // ignore first char which is {
  memcpy(scripts[scriptnum]->data, current_section->start+1, datasize);

  // tack on a NULL to end the string
  scripts[scriptnum]->data[datasize] = NULL;
  
  scripts[scriptnum]->scriptnum = scriptnum;
  scripts[scriptnum]->parent = current_script; // remember parent
  
  // preprocess the script now
  preprocess(scripts[scriptnum]);
    
  // restore current_script: usefully stored in new script
  current_script = scripts[scriptnum]->parent;

  // rover may also be changed, but is changed below anyway
  
  // we dont want to run the script, only add it
  // jump past the script in parsing
  
  rover = current_section->end + 1;
}



/****** scripting command list *******/

void T_AddCommands()
{
  C_AddCommand(t_dump);
  C_AddCommand(t_run);
  C_AddCommand(t_running);
}
