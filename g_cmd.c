// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
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
// Game console commands
//
// Console commands controlling the game functions.
//
// By Simon Howard
//
//---------------------------------------------------------------------------

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "c_net.h"
#include "f_wipe.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "mn_engin.h"
#include "m_random.h"
#include "p_inter.h"
#include "p_setup.h"
#include "w_wad.h"
#include "z_zone.h"

extern int automlook;
extern int invert_mouse;
extern int screenshot_pcx;
extern boolean sendpause;
extern int forwardmove[2];
extern int sidemove[2];

////////////////////////////////////////////////////////////////////////
//
// Game Commands
//

CONSOLE_COMMAND(i_error, 0)
{
  I_Error(c_args);
}

CONSOLE_COMMAND(starttitle, cf_notnet)
{
  MN_ClearMenus();         // put menu away
  D_StartTitle();
}

CONSOLE_COMMAND(endgame, cf_notnet)
{
  C_SetConsole();
}

CONSOLE_COMMAND(pause, cf_server)
{
  sendpause = true;
}

CONSOLE_COMMAND(quit, 0)
{
  exit(0);
}

CONSOLE_COMMAND(animshot, 0)
{
  if(!c_argc)
    {
      C_Printf(
	       "animated screenshot.\n"
	       "usage: animshot <frames>\n");
      return;
    }
  animscreenshot = atoi(c_argv[0]);
  current_height = current_target = 0;    // turn off console
}

// always mlook

VARIABLE_BOOLEAN(automlook, NULL,           onoff);
CONSOLE_VARIABLE(alwaysmlook, automlook, 0) {}

// invert mouse

VARIABLE_BOOLEAN(invert_mouse, NULL,        onoff);
CONSOLE_VARIABLE(invertmouse, invert_mouse, 0) {}

// horizontal mouse sensitivity

VARIABLE_INT(mouseSensitivity_horiz, NULL, 0, 64, NULL);
CONSOLE_VARIABLE(sens_horiz, mouseSensitivity_horiz, 0) {}

// vertical mouse sensitivity

VARIABLE_INT(mouseSensitivity_vert, NULL, 0, 100, NULL);
CONSOLE_VARIABLE(sens_vert, mouseSensitivity_vert, 0) {}

// player bobbing

VARIABLE_BOOLEAN(player_bobbing, NULL,      onoff);
CONSOLE_VARIABLE(bobbing, player_bobbing, 0) {}

// turbo scale

int turbo_scale = 100;
VARIABLE_INT(turbo_scale, NULL,         10, 400, NULL);
CONSOLE_VARIABLE(turbo, turbo_scale, cf_nosave)
{
  C_Printf ("turbo scale: %i%%\n",turbo_scale);
  forwardmove[0] = (0x19*turbo_scale)/100;
  forwardmove[1] = (0x32*turbo_scale)/100;
  sidemove[0] = (0x18*turbo_scale)/100;
  sidemove[1] = (0x28*turbo_scale)/100;
}

CONSOLE_NETCMD(exitlevel, cf_server|cf_level, netcmd_exitlevel)
{
  G_ExitLevel();
}

//////////////////////////////////////
//
// Demo Stuff
//

CONSOLE_COMMAND(stopdemo, cf_notnet)
{
  G_StopDemo();
}

CONSOLE_COMMAND(timedemo, cf_notnet)
{
  G_TimeDemo(c_argv[0]);
}

// 'cool' demo

VARIABLE_BOOLEAN(cooldemo, NULL,            onoff);
CONSOLE_VARIABLE(cooldemo, cooldemo, cf_nosave) {}

///////////////////////////////////////////////////
//
// Wads
//

// load new wad
// buffered command: r_init during load

CONSOLE_COMMAND(addfile, cf_notnet|cf_buffered)
{
  D_AddNewFile(c_argv[0]);
}

// list loaded wads

CONSOLE_COMMAND(listwads, 0)
{
  D_ListWads();
}
                                     
// random seed

CONST_INT(rngseed);
CONSOLE_CONST(rngseed, rngseed);

// suicide

CONSOLE_NETCMD(kill, cf_level, netcmd_kill)
{
  mobj_t *mobj;
  int playernum;
  
  playernum = cmdsrc;
  
  mobj = players[playernum].mo;
  P_DamageMobj(mobj, NULL, mobj,
	       2*(players[playernum].health+players[playernum].armorpoints) );
  mobj->momx = mobj->momy = mobj->momz = 0;
}

// change level

CONST_STRING(levelmapname);
CONSOLE_CONST(mapname, levelmapname);

CONSOLE_NETCMD(map, cf_buffered|cf_server, netcmd_map)
{
  if(!c_argc)
    {
      C_Printf("usage: map <mapname>\n"
               "   or map <wadfile.wad>\n");
      return;
    }
  
  G_StopDemo();
  
  // check for .wad files
  // i'm not particularly a fan of this myself, but..
  
  if(strlen(c_argv[0]) > 4)
    {
      char *extension;
      extension = c_argv[0] + strlen(c_argv[0]) - 4;
      if(!strcmp(extension, ".wad"))
	{
	  if(D_AddNewFile(c_argv[0]))
	    {
	      if(wad_level)     // new wad contains level(s)
		G_InitNew(gameskill, firstlevel);
	      else
		D_StartTitle();   // go to title
	    }
	  return;
	}
    }
  
  G_InitNew(gameskill, c_argv[0]);
}

// player name
VARIABLE_STRING(default_name, NULL,             18);
CONSOLE_NETVAR(name, default_name, cf_handlerset, netcmd_name)
{
  int playernum;
  
  playernum = cmdsrc;
  
  strncpy(players[playernum].name, c_args, 18);
  if(playernum == consoleplayer)
    {
      free(default_name);
      default_name = strdup(c_args);
    }
}

// screenshot type

char *str_pcx[] = {"bmp", "pcx"};
VARIABLE_BOOLEAN(screenshot_pcx, NULL,     str_pcx);
CONSOLE_VARIABLE(shot_type,     screenshot_pcx, 0) {}

// textmode startup

extern int textmode_startup;            // d_main.c
VARIABLE_BOOLEAN(textmode_startup, NULL,        onoff);
CONSOLE_VARIABLE(textmode_startup, textmode_startup, 0) {}

// demo insurance

extern int demo_insurance;
char *insure_str[]={"off", "on", "when recording"};
VARIABLE_INT(demo_insurance, &default_demo_insurance, 0, 2, insure_str);
CONSOLE_VARIABLE(demo_insurance, demo_insurance, cf_notnet) {}

extern int smooth_turning;
VARIABLE_BOOLEAN(smooth_turning, NULL,          onoff);
CONSOLE_VARIABLE(smooth_turning, smooth_turning, 0) {}

////////////////////////////////////////////////////////////////
//
// Chat Macros
//

void G_AddChatMacros()
{
  int i;

  for(i=0; i<10; i++)
    {
      variable_t *variable;
      command_t *command;
      char tempstr[10];
      
      // create the variable first
      variable = malloc(sizeof(*variable));
      variable->variable = &chat_macros[i];
      variable->v_default = NULL;
      variable->type = vt_string;      // string value
      variable->min = 0;
      variable->max = 128;              // 40 chars is enough
      variable->defines = NULL;

      // now the command
      command = malloc(sizeof(*command));

      sprintf(tempstr, "chatmacro%i", i);
      command->name = strdup(tempstr);
      command->type = ct_variable;
      command->flags = 0;
      command->variable = variable;
      command->handler = NULL;
      command->netcmd = 0;

      (C_AddCommand)(command); // hook into cmdlist
    }
}

///////////////////////////////////////////////////////////////
//
// Weapon Prefs
//

extern int weapon_preferences[2][NUMWEAPONS+1];                   

void G_SetWeapPref(int prefnum, int newvalue)
{
  int i;
  
  // find the pref which has the new value
  
  for(i=0; i<NUMWEAPONS; i++)
    if(weapon_preferences[0][i] == newvalue) break;
  
  weapon_preferences[0][i] = weapon_preferences[0][prefnum];
  weapon_preferences[0][prefnum] = newvalue;
}

char *weapon_str[] =
{"fist", "pistol", "shotgun", "chaingun", "rocket launcher", "plasma gun",
 "bfg", "chainsaw", "double shotgun"};

void G_WeapPrefHandler()
{
  int prefnum = (int *)c_command->variable->variable - weapon_preferences[0];
  G_SetWeapPref(prefnum, atoi(c_argv[0]));
}

void G_AddWeapPrefs()
{
  int i;

  for(i=0; i<9; i++)
    {
      variable_t *variable;
      command_t *command;
      char tempstr[10];
      
      // create the variable first
      variable = malloc(sizeof(*variable));
      variable->variable = &weapon_preferences[0][i];
      variable->v_default = NULL;
      variable->type = vt_int;
      variable->min = 1;
      variable->max = 9;
      variable->defines = weapon_str;  // use weapon string defines

      // now the command
      command = malloc(sizeof(*command));

      sprintf(tempstr, "weappref_%i", i+1);
      command->name = strdup(tempstr);
      command->type = ct_variable;
      command->flags = cf_handlerset;
      command->variable = variable;
      command->handler = G_WeapPrefHandler;
      command->netcmd = 0;

      (C_AddCommand)(command); // hook into cmdlist
    }
}

///////////////////////////////////////////////////////////////
//
// Compatibility vectors
//

// names given to cmds
const char *comp_strings[] =
{
  "telefrag",
  "dropoff",
  "vile",
  "pain",
  "skull",
  "blazing",
  "doorlight",
  "model",
  "god",
  "falloff",
  "floors",
  "skymap",
  "pursuit",
  "doorstuck",
  "staylift",
  "zombie",
  "stairs",
  "infcheat",
  "zerotags",
};

void G_AddCompat()
{
  int i;

  for(i=0; i<=comp_zerotags; i++)
    {
      variable_t *variable;
      command_t *command;
      char tempstr[20];
      
      // create the variable first
      variable = malloc(sizeof(*variable));
      variable->variable = &comp[i];
      variable->v_default = &default_comp[i];
      variable->type = vt_int;      // string value
      variable->min = 0;
      variable->max = 1;
      variable->defines = yesno;

      // now the command
      command = malloc(sizeof(*command));

      sprintf(tempstr, "comp_%s", comp_strings[i]);
      command->name = strdup(tempstr);
      command->type = ct_variable;
      command->flags = cf_server | cf_netvar;
      command->variable = variable;
      command->handler = NULL;
      command->netcmd = netcmd_comp_0 + i;

      (C_AddCommand)(command); // hook into cmdlist
    }
}

// wad/dehs loaded at startup

char *wadfile_1 = NULL, *wadfile_2 = NULL;

VARIABLE_STRING(wadfile_1, NULL,       30); 
CONSOLE_VARIABLE(wadfile_1, wadfile_1, 0) {}

VARIABLE_STRING(wadfile_2, NULL,       30); 
CONSOLE_VARIABLE(wadfile_2, wadfile_2, 0) {}

char *dehfile_1 = NULL, *dehfile_2 = NULL;

VARIABLE_STRING(dehfile_1, NULL,       30); 
CONSOLE_VARIABLE(dehfile_1, dehfile_1, 0) {}

VARIABLE_STRING(dehfile_2, NULL,       30); 
CONSOLE_VARIABLE(dehfile_2, dehfile_2, 0) {}

extern void G_Bind_AddCommands();

void G_AddCommands()
{
  C_AddCommand(i_error);
  C_AddCommand(starttitle);
  C_AddCommand(endgame);
  C_AddCommand(pause);
  C_AddCommand(quit);
  C_AddCommand(animshot);
  C_AddCommand(shot_type);
  C_AddCommand(alwaysmlook);
  C_AddCommand(bobbing);
  C_AddCommand(sens_vert);
  C_AddCommand(sens_horiz);
  C_AddCommand(invertmouse);
  C_AddCommand(turbo);
  C_AddCommand(timedemo);
  C_AddCommand(cooldemo);
  C_AddCommand(stopdemo);
  C_AddCommand(exitlevel);
  C_AddCommand(addfile);
  C_AddCommand(listwads);
  C_AddCommand(rngseed);
  C_AddCommand(kill);
  C_AddCommand(map);
  C_AddCommand(mapname);
  C_AddCommand(name);
  C_AddCommand(textmode_startup);
  C_AddCommand(demo_insurance);
  C_AddCommand(smooth_turning);

  C_AddCommand(wadfile_1);
  C_AddCommand(wadfile_2);
  C_AddCommand(dehfile_1);
  C_AddCommand(dehfile_2);
  
  G_AddChatMacros();
  G_AddWeapPrefs();
  G_AddCompat();

  G_Bind_AddCommands();
}
