// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// Key Bindings
//
// Rather than use the old system of binding a key to an action, we bind
// an action to a key. This way we can have as many actions as we want
// bound to a particular key.
//
// By Simon Howard
//
//---------------------------------------------------------------------------

#include "doomdef.h"
#include "doomstat.h"
#include "z_zone.h"

#include "c_io.h"
#include "c_runcmd.h"
#include "g_game.h"
#include "mn_engin.h"
#include "m_misc.h"

// action variables

int action_forward;
int action_backward;
int action_left;
int action_right;
int action_moveleft;
int action_moveright;
int action_use;
int action_speed;
int action_attack;
int action_strafe;
int action_flip;

int action_mlook;
int action_lookup;
int action_lookdown;
int action_center;

int action_weapon1;
int action_weapon2;
int action_weapon3;
int action_weapon4;
int action_weapon5;
int action_weapon6;
int action_weapon7;
int action_weapon8;
int action_weapon9;

int action_nextweapon;

int action_frags;

int autorun = false;

//===========================================================================
//
// Functions
//
//===========================================================================

// autorun toggle
void Action_Autorun()
{
  C_Printf("autorun toggle\n");
  autorun = !autorun;
}

// actions list

typedef struct
{
  char *name;        // text description
  enum
    {
      at_variable,
      at_function,
      at_conscmd,       // console command
    } type;
  union
  {
    int *variable;   // variable -- if non-zero, action activated (key down)
    void (*Handler)();
  } value;
} keyaction_t;

keyaction_t keyactions[] =
  {
    {"forward",            at_variable,     {&action_forward}},
      {"backward",         at_variable,     {&action_backward}},
      {"left",             at_variable,     {&action_left}},
      {"right",            at_variable,     {&action_right}},
      {"moveleft",         at_variable,     {&action_moveleft}},
      {"moveright",        at_variable,     {&action_moveright}},
      {"use",              at_variable,     {&action_use}},
      {"strafe",           at_variable,     {&action_strafe}},
      {"attack",           at_variable,     {&action_attack}},
      {"flip",             at_variable,     {&action_flip}},
      {"speed",            at_variable,     {&action_speed}},

      {"mlook",            at_variable,     {&action_mlook}},
      {"lookup",           at_variable,     {&action_lookup}},
      {"lookdown",         at_variable,     {&action_lookdown}},
      {"center",           at_variable,     {&action_center}},
    
      {"weapon1",          at_variable,     {&action_weapon1}},
      {"weapon2",          at_variable,     {&action_weapon2}},
      {"weapon3",          at_variable,     {&action_weapon3}},
      {"weapon4",          at_variable,     {&action_weapon4}},
      {"weapon5",          at_variable,     {&action_weapon5}},
      {"weapon6",          at_variable,     {&action_weapon6}},
      {"weapon7",          at_variable,     {&action_weapon7}},
      {"weapon8",          at_variable,     {&action_weapon8}},
      {"weapon9",          at_variable,     {&action_weapon9}},
      {"nextweapon",       at_variable,     {&action_nextweapon}},

      {"frags",            at_variable,     {&action_frags}},
    
      {"autorun",          at_function,     {(int *)&Action_Autorun}},
  };

const int num_keyactions = sizeof(keyactions) / sizeof(*keyactions);


// console bindings are stored seperately
// and are added to list dynamically

static int cons_keyactions_alloced;
static keyaction_t *cons_keyactions = NULL;
static int num_cons_keyactions;

// key bindings

#define NUM_KEYS 256

typedef struct
{
  char *name;
  boolean keydown;
  keyaction_t *binding;
} doomkey_t;

static doomkey_t keys[NUM_KEYS];

//-----------------------------------------------------------------------
//
// G_KeyActionForName
//
// Obtain a keyaction from its name
//

static keyaction_t *G_KeyActionForName(char *name)
{
  int i;

  // sequential search
  // this is only called every now and then

  for(i=0; i<num_keyactions; i++)
    if(!strcasecmp(name, keyactions[i].name))
      return &keyactions[i];

  // check console keyactions

  if(cons_keyactions)
    for(i=0; i<num_cons_keyactions; i++)
      {
	if(!strcasecmp(name, cons_keyactions[i].name))
	  return &cons_keyactions[i];
      }
  else
    {
      cons_keyactions_alloced = 128;
      num_cons_keyactions = 0;
      cons_keyactions =
	Z_Malloc((cons_keyactions_alloced+3) * sizeof(*cons_keyactions),
		 PU_STATIC, 0);
    }
  
  // not in list: add to list

  // increase list size if neccesary
  
  if(num_cons_keyactions >= cons_keyactions_alloced)
    {
      cons_keyactions_alloced *= 2;
      cons_keyactions =
	Z_Realloc(cons_keyactions,
		  (cons_keyactions_alloced+3) * sizeof(*cons_keyactions),
		  PU_STATIC, 0);
    }

  // add to list

  //  C_Printf("add %s\n", name);
  
  cons_keyactions[num_cons_keyactions].type = at_conscmd;
  cons_keyactions[num_cons_keyactions].name = Z_Strdup(name, PU_STATIC, 0);

  return &cons_keyactions[num_cons_keyactions++];
}

//--------------------------------------------------------------------------
//
// G_KeyForName
//
// Obtain a keyaction from its name
//

static int G_KeyForName(char *name)
{
  int i;

  for(i=0; i<NUM_KEYS; i++)
    if(!strcasecmp(keys[i].name, name))
      return tolower(i);

  return -1;
}

//--------------------------------------------------------------------------
//
// G_BindKeyToAction
//

static void G_BindKeyToAction(char *key_name, char *action_name)
{
  int key;
  keyaction_t *action;
  
  // get key
  
  key = G_KeyForName(key_name);

  if(key < 0)
    {
      C_Printf("unknown key '%s'\n", key_name);
      return;
    }

  // get action
  
  action = G_KeyActionForName(action_name);

  if(!action)
    {
      C_Printf("unknown action '%s'\n", action_name);
      return;
    }

  keys[key].binding = action;

  //  C_Printf("%s bound to %s\n", key_name, action_name);
}

//==========================================================================
//
// Init.
//
// Set up key names etc.
//
//==========================================================================

void G_InitKeyBindings()
{
  int i;
  
  // various names for different keys
  
  keys[KEYD_RIGHTARROW].name  = "rightarrow";
  keys[KEYD_LEFTARROW].name   = "leftarrow";
  keys[KEYD_UPARROW].name     = "uparrow";
  keys[KEYD_DOWNARROW].name   = "downarrow";
  keys[KEYD_ESCAPE].name      = "escape";
  keys[KEYD_ENTER].name       = "enter";
  keys[KEYD_TAB].name         = "tab";

  keys[KEYD_F1].name          = "f1";
  keys[KEYD_F2].name          = "f2";
  keys[KEYD_F3].name          = "f3";
  keys[KEYD_F4].name          = "f4";
  keys[KEYD_F5].name          = "f5";
  keys[KEYD_F6].name          = "f6";
  keys[KEYD_F7].name          = "f7";
  keys[KEYD_F8].name          = "f8";
  keys[KEYD_F9].name          = "f9";
  keys[KEYD_F10].name         = "f10";
  keys[KEYD_F11].name         = "f11";
  keys[KEYD_F12].name         = "f12";

  keys[KEYD_BACKSPACE].name   = "backspace";
  keys[KEYD_PAUSE].name       = "pause";
  keys[KEYD_MINUS].name       = "-";
  keys[KEYD_RSHIFT].name      = "shift";
  keys[KEYD_RCTRL].name       = "ctrl";
  keys[KEYD_RALT].name        = "alt";
  keys[KEYD_CAPSLOCK].name    = "capslock";

  keys[KEYD_INSERT].name      = "insert";
  keys[KEYD_HOME].name        = "home";
  keys[KEYD_END].name         = "end";
  keys[KEYD_PAGEUP].name      = "pgup";
  keys[KEYD_PAGEDOWN].name    = "pgdn";
  keys[KEYD_SCROLLLOCK].name  = "scrolllock";
  keys[KEYD_SPACEBAR].name    = "space";
  keys[KEYD_NUMLOCK].name     = "numlock";

  keys[KEYD_MOUSE1].name      = "mouse1";
  keys[KEYD_MOUSE2].name      = "mouse2";
  keys[KEYD_MOUSE3].name      = "mouse3";
  
  keys[KEYD_JOY1].name        = "joy1";
  keys[KEYD_JOY2].name        = "joy2";
  keys[KEYD_JOY3].name        = "joy3";
  keys[KEYD_JOY4].name        = "joy4";

  keys[','].name = "<";
  keys['.'].name = ">";
  
  for(i=0; i<NUM_KEYS; i++)
    {
      // fill in name if not set yet
      
      if(!keys[i].name)
	{
	  char tempstr[32];

	  // build generic name
	  if(isprint(i))
	    sprintf(tempstr, "%c", i);
	  else
	    sprintf(tempstr, "key%02i", i);
	  
	  keys[i].name = Z_Strdup(tempstr, PU_STATIC, 0);
	}

      keys[i].binding = NULL;
    }
  
  //  G_SetDefaultBindings();
}

//-------------------------------------------------------------------------
//
// G_KeyResponder
//

boolean G_KeyResponder(event_t *ev)
{
  if(ev->type == ev_keydown)
    {
      int key = tolower(ev->data1);

      if(!keys[key].keydown)
	{
	  keys[key].keydown = true;
	  
	  if(keys[key].binding)
	    {
	      switch(keys[key].binding->type)
		{
		  case at_variable:
		    (*keys[key].binding->value.variable)++;
		    //		    C_Printf("%s: %i\n",
		    //			     keys[key].binding->name,
		    //			     *keys[key].binding->value.variable);
		    break;

		  case at_function:
		    keys[key].binding->value.Handler();
		    break;

		  case at_conscmd:
		    C_RunTextCmd(keys[key].binding->name);
		    break;
		    
		  default:
		    break;
		}
	    }
	}
    }

  if(ev->type == ev_keyup)
    {
      int key = tolower(ev->data1);

      keys[key].keydown = false;
      
      if(keys[key].binding)
	{
	  switch(keys[key].binding->type)
	    {
	      case at_variable:
		if(*keys[key].binding->value.variable > 0)
		  (*keys[key].binding->value.variable)--;

		//		C_Printf("%s: %i\n",
		//			 keys[key].binding->name,
		//			 *keys[key].binding->value.variable);
		break;
		
	      case at_function:
		break;
		
	      default:
		break;
	    }
	}
    }

  return true;
}

//---------------------------------------------------------------------------
//
// G_BoundKeys
//
// Get an ascii description of the keys bound to a particular action
//

char *G_BoundKeys(char *action)
{
  keyaction_t *ke = G_KeyActionForName(action);
  int i;
  static char ret[100];   // store list of keys bound to this
  
  if(!ke)
    return "coded by a m0f0";

  ret[0] = '\0';   // clear ret
  
  // sequential search -ugh

  for(i=0; i<NUM_KEYS; i++)
    {
      if(keys[i].binding == ke)
	{
	  if(ret[0])
	    strcat(ret, " + ");
	  strcat(ret, keys[i].name);
	}
    }
  
  return ret[0] ? ret : "none";
}


CONSOLE_COMMAND(bind, 0)
{
  if(c_argc >= 2)
    {
      G_BindKeyToAction(c_argv[0], c_argv[1]);
    }
  else if(c_argc == 1)
    {
      int key = G_KeyForName(c_argv[0]);
      if(key < 0)
	C_Printf("no such key!\n");
      else
	{
	  if(keys[key].binding)
	    C_Printf("%s bound to %s\n", keys[key].name,
		     keys[key].binding->name);
	  else
	    C_Printf("%s not bound\n", keys[key].name);
	}
    }
  else
    {
      C_Printf("usage: bind key action\n");
    }
}

//===========================================================================
//
// Binding selection widget
//
// For menu: when we select to change a key binding the widget is used
// as the drawer and responder
//
//===========================================================================

static char *binding_action;       // name of action we are editing

//
// G_BindDrawer
//
// Draw the prompt box
//

void G_BindDrawer()
{
  char temp[100];
  int wid, height;
  
  // draw the menu in the background

  MN_DrawMenu(current_menu);

  // create message
  
  strcpy(temp, "\n -= input new key =- \n");
  
  wid = V_StringWidth(temp);
  height = V_StringHeight(temp);

  // draw box
  
  V_DrawBox((SCREENWIDTH - wid) / 2 - 4,
	    (SCREENHEIGHT - height) / 2 - 4,
	    wid + 8,
	    height + 8);

  // write text in box

  V_WriteText(temp,
	      (SCREENWIDTH - wid) / 2,
	      (SCREENHEIGHT - height) / 2);
}

//
// G_BindResponder
//
// Responder for widget
//

boolean G_BindResponder(event_t *ev)
{
  keyaction_t *action;
  int i;
  int bound;
  
  if(ev->type != ev_keydown)
    return false;

  if(ev->data1 == KEYD_ESCAPE)    // cancel
    {
      current_menuwidget = NULL;
      return true;
    }

  // got a key - close box
  current_menuwidget = NULL;
  
  action = G_KeyActionForName(binding_action);
  if(!action)
    {
      C_Printf("unknown binding '%s'\n", binding_action);
      return true;
    }

  // find how many keys bound

  bound = 0;
  for(i=0; i<NUM_KEYS; i++)
    bound += (keys[i].binding == action);

  // if already 2 or more then clear them
  
  if(bound >= 2)
    for(i=0; i<NUM_KEYS; i++)
      if(keys[i].binding == action)
	keys[i].binding = NULL;

  // bind new key to action

  keys[ev->data1].binding = action;

  return true;
}

menuwidget_t binding_widget = {G_BindDrawer, G_BindResponder};

//
// G_EditBinding
//
// Main Function
//

void G_EditBinding(char *action)
{
  current_menuwidget = &binding_widget;
  binding_action = action;
}

//===========================================================================
//
// Load/Save defaults
//
//===========================================================================

// default script:

static char *cfg_file = NULL; 
char *default_script =
"name \"player\"\n"
"skin \"marine\"\n"
"c_speed 200\n"
"inet_server \"62.252.3.24\"\n"
"bind tab togglemap\n"
"bind space use\n"
"bind * screenshot\n"
"bind < moveleft\n"
"bind - \"screensize -\"\n"
"bind > moveright\n"
"bind / nextweapon\n"
"bind 1 weapon1\n"
"bind 2 weapon2\n"
"bind 3 weapon3\n"
"bind 4 weapon4\n"
"bind 5 weapon5\n"
"bind 6 weapon6\n"
"bind 7 weapon7\n"
"bind 8 weapon8\n"
"bind 9 weapon9\n"
"bind = \"screensize +\"\n"
"bind ctrl attack\n"
"bind leftarrow left\n"
"bind uparrow forward\n"
"bind rightarrow right\n"
"bind downarrow backward\n"
"bind shift speed\n"
"bind alt strafe\n"
"bind f1 help\n"
"bind f2 mn_loadgame\n"
"bind f3 mn_savegame\n"
"bind f4 mn_sound\n"
"bind f5 \"hu_overlay /\"\n"
"bind f7 mn_endgame\n"
"bind f8 \"messages /\"\n"
"bind f10 mn_quit\n"
"bind home center\n"
"bind pgup lookup\n"
"bind pgdn lookdown\n"
"bind f11 \"gamma /\"\n"
"bind f12 spy\n"
"bind mouse1 attack\n"
"bind mouse2 forward\n"
"bind pause pause\n"
;

void G_LoadDefaults(char *file)
{
  byte *cfg_data;

  cfg_file = strdup(file);
  
  if(M_ReadFile(cfg_file, &cfg_data) <= 0)
    {
      cfg_data = default_script;
    }
  
  C_RunScript(cfg_data);
  
  //  if(cfg_file != default_script)
  //    Z_Free(cfg_file);
}

void G_SaveDefaults()
{
  FILE *file;
  command_t *cmd;
  int i;

  if(!cfg_file)         // check defaults have been loaded
    return;
  
  file = fopen(cfg_file, "w");

  // write console variables
  
  for(i=0; i<CMDCHAINS; i++)
    {
      for(cmd = cmdroots[i]; cmd; cmd = cmd->next)
	{
	  if(cmd->type != ct_variable)    // only write variables
	    continue;
	  if(cmd->flags & cf_nosave)      // do not save if cf_nosave set
	    continue;
	  
	  fprintf(file, "%s \"%s\"\n",
		  cmd->name,
		  C_VariableValue(cmd->variable));
	}
    }

  // write key bindings

  for(i=0; i<NUM_KEYS; i++)
    {
      if(keys[i].binding)
	{
	  fprintf(file, "bind %s \"%s\"\n",
		keys[i].name,
		keys[i].binding->name);
	}
    }

  
  fclose(file);
}

//===========================================================================
//
// Console Commands
//
//===========================================================================

void G_Bind_AddCommands()
{
  C_AddCommand(bind);
}
