// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Game console commands
//
// console commands controlling the game functions.
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
#include "w_wad.h"

/****** externs ******/

extern int automlook;
extern int invert_mouse;
extern int screenshot_pcx;
extern boolean sendpause;
extern int forwardmove[2];
extern int sidemove[2];

/******* game commands *******/

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

VARIABLE_INT(mouseSensitivity_vert, NULL, 0, 48, NULL);
CONSOLE_VARIABLE(sens_vert, mouseSensitivity_vert, 0) {}

// player bobbing

VARIABLE_BOOLEAN(player_bobbing, NULL,      onoff);
CONSOLE_VARIABLE(bobbing, player_bobbing, 0) {}

// turbo scale

int turbo_scale = 100;
VARIABLE_INT(turbo_scale, NULL,         10, 400, NULL);
CONSOLE_VARIABLE(turbo, turbo_scale, 0)
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

          /******* demo stuff *********/

CONSOLE_COMMAND(playdemo, cf_notnet)
{
  if(W_CheckNumForName(c_argv[0]) == -1)
    {
      C_Printf("%s not found\n",c_argv[0]);
      return;
    }
  G_DeferedPlayDemo(c_argv[0]);
  singledemo = true;            // quit after one demo
}


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
CONSOLE_VARIABLE(cooldemo, cooldemo, 0) {}

    /**************** wads ****************/

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

CONSOLE_NETCMD(map, cf_server, netcmd_map)
{
  if(!c_argc)
    {
      C_Printf("usage: map <mapname>\n");
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
	      G_InitNew(gameskill, startlevel);
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
  
  strncpy(players[playernum].name, c_argv[0], 18);
  if(playernum == consoleplayer)
    {
      free(default_name);
      default_name = strdup(c_argv[0]);
    }
}


// screen wipe speed

VARIABLE_INT(wipe_speed, NULL,                  1, 200, NULL);
CONSOLE_VARIABLE(wipe_speed, wipe_speed, 0) {}

// screenshot type

char *str_pcx[] = {"bmp", "pcx"};
VARIABLE_BOOLEAN(screenshot_pcx, NULL,     str_pcx);
CONSOLE_VARIABLE(shot_type,     screenshot_pcx, 0) {}

// textmode startup

extern int textmode_startup;            // d_main.c
VARIABLE_BOOLEAN(textmode_startup, NULL,        onoff);
CONSOLE_VARIABLE(textmode_startup, textmode_startup, 0) {}

        /********* chat macros ************/

        // must be done with variable_t :(
variable_t var_chatmacro0 = {&chat_macros[0], NULL, vt_string, 0, 128, NULL};
variable_t var_chatmacro1 = {&chat_macros[1], NULL, vt_string, 0, 128, NULL};
variable_t var_chatmacro2 = {&chat_macros[2], NULL, vt_string, 0, 128, NULL};
variable_t var_chatmacro3 = {&chat_macros[3], NULL, vt_string, 0, 128, NULL};
variable_t var_chatmacro4 = {&chat_macros[4], NULL, vt_string, 0, 128, NULL};
variable_t var_chatmacro5 = {&chat_macros[5], NULL, vt_string, 0, 128, NULL};
variable_t var_chatmacro6 = {&chat_macros[6], NULL, vt_string, 0, 128, NULL};
variable_t var_chatmacro7 = {&chat_macros[7], NULL, vt_string, 0, 128, NULL};
variable_t var_chatmacro8 = {&chat_macros[8], NULL, vt_string, 0, 128, NULL};
variable_t var_chatmacro9 = {&chat_macros[9], NULL, vt_string, 0, 128, NULL};

CONSOLE_VARIABLE(chatmacro0, chatmacro0, 0) {}
CONSOLE_VARIABLE(chatmacro1, chatmacro1, 0) {}
CONSOLE_VARIABLE(chatmacro2, chatmacro2, 0) {}
CONSOLE_VARIABLE(chatmacro3, chatmacro3, 0) {}
CONSOLE_VARIABLE(chatmacro4, chatmacro4, 0) {}
CONSOLE_VARIABLE(chatmacro5, chatmacro5, 0) {}
CONSOLE_VARIABLE(chatmacro6, chatmacro6, 0) {}
CONSOLE_VARIABLE(chatmacro7, chatmacro7, 0) {}
CONSOLE_VARIABLE(chatmacro8, chatmacro8, 0) {}
CONSOLE_VARIABLE(chatmacro9, chatmacro9, 0) {}

        /********** weapon prefs **************/

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

variable_t var_weaponpref1 = {&weapon_preferences[0][0], NULL, vt_int, 1,9, weapon_str};
variable_t var_weaponpref2 = {&weapon_preferences[0][1], NULL, vt_int, 1,9, weapon_str};
variable_t var_weaponpref3 = {&weapon_preferences[0][2], NULL, vt_int, 1,9, weapon_str};
variable_t var_weaponpref4 = {&weapon_preferences[0][3], NULL, vt_int, 1,9, weapon_str};
variable_t var_weaponpref5 = {&weapon_preferences[0][4], NULL, vt_int, 1,9, weapon_str};
variable_t var_weaponpref6 = {&weapon_preferences[0][5], NULL, vt_int, 1,9, weapon_str};
variable_t var_weaponpref7 = {&weapon_preferences[0][6], NULL, vt_int, 1,9, weapon_str};
variable_t var_weaponpref8 = {&weapon_preferences[0][7], NULL, vt_int, 1,9, weapon_str};
variable_t var_weaponpref9 = {&weapon_preferences[0][8], NULL, vt_int, 1,9, weapon_str};

CONSOLE_VARIABLE(weappref_1, weaponpref1, cf_handlerset) {G_SetWeapPref(0, atoi(c_argv[0]));}
CONSOLE_VARIABLE(weappref_2, weaponpref2, cf_handlerset) {G_SetWeapPref(1, atoi(c_argv[0]));}
CONSOLE_VARIABLE(weappref_3, weaponpref3, cf_handlerset) {G_SetWeapPref(2, atoi(c_argv[0]));}
CONSOLE_VARIABLE(weappref_4, weaponpref4, cf_handlerset) {G_SetWeapPref(3, atoi(c_argv[0]));}
CONSOLE_VARIABLE(weappref_5, weaponpref5, cf_handlerset) {G_SetWeapPref(4, atoi(c_argv[0]));}
CONSOLE_VARIABLE(weappref_6, weaponpref6, cf_handlerset) {G_SetWeapPref(5, atoi(c_argv[0]));}
CONSOLE_VARIABLE(weappref_7, weaponpref7, cf_handlerset) {G_SetWeapPref(6, atoi(c_argv[0]));}
CONSOLE_VARIABLE(weappref_8, weaponpref8, cf_handlerset) {G_SetWeapPref(7, atoi(c_argv[0]));}
CONSOLE_VARIABLE(weappref_9, weaponpref9, cf_handlerset) {G_SetWeapPref(8, atoi(c_argv[0]));}

//      compatibility vectors
// ugh

variable_t var_comp_telefrag  = {&comp[comp_telefrag],  &default_comp[comp_telefrag], vt_int, 0, 1, yesno};
variable_t var_comp_dropoff   = {&comp[comp_dropoff],   &default_comp[comp_dropoff], vt_int, 0, 1, yesno};
variable_t var_comp_vile      = {&comp[comp_vile],      &default_comp[comp_vile], vt_int, 0, 1, yesno};
variable_t var_comp_pain      = {&comp[comp_pain],      &default_comp[comp_pain], vt_int, 0, 1, yesno};
variable_t var_comp_skull     = {&comp[comp_skull],     &default_comp[comp_skull], vt_int, 0, 1, yesno};
variable_t var_comp_blazing   = {&comp[comp_blazing],   &default_comp[comp_blazing], vt_int, 0, 1, yesno};
variable_t var_comp_doorlight = {&comp[comp_doorlight], &default_comp[comp_doorlight], vt_int, 0, 1, yesno};
variable_t var_comp_model     = {&comp[comp_model],     &default_comp[comp_model], vt_int, 0, 1, yesno};
variable_t var_comp_god       = {&comp[comp_god],       &default_comp[comp_god], vt_int, 0, 1, yesno};
variable_t var_comp_falloff   = {&comp[comp_falloff],   &default_comp[comp_falloff], vt_int, 0, 1, yesno};
variable_t var_comp_floors    = {&comp[comp_floors],    &default_comp[comp_floors], vt_int, 0, 1, yesno};
variable_t var_comp_skymap    = {&comp[comp_skymap],    &default_comp[comp_skymap], vt_int, 0, 1, yesno};
variable_t var_comp_pursuit   = {&comp[comp_pursuit],   &default_comp[comp_pursuit], vt_int, 0, 1, yesno};
variable_t var_comp_doorstuck = {&comp[comp_doorstuck], &default_comp[comp_doorstuck], vt_int, 0, 1, yesno};
variable_t var_comp_staylift  = {&comp[comp_staylift],  &default_comp[comp_staylift], vt_int, 0, 1, yesno};
variable_t var_comp_zombie    = {&comp[comp_zombie],    &default_comp[comp_zombie], vt_int, 0, 1, yesno};
variable_t var_comp_stairs    = {&comp[comp_stairs],    &default_comp[comp_stairs], vt_int, 0, 1, yesno};
variable_t var_comp_infcheat  = {&comp[comp_infcheat],  &default_comp[comp_infcheat], vt_int, 0, 1, yesno};
variable_t var_comp_zerotags  = {&comp[comp_zerotags],  &default_comp[comp_zerotags], vt_int, 0, 1, yesno};

CONSOLE_NETVAR(comp_telefrag,  comp_telefrag,  cf_server, netcmd_comp_0) {}
CONSOLE_NETVAR(comp_dropoff,   comp_dropoff,   cf_server, netcmd_comp_1) {}
CONSOLE_NETVAR(comp_vile,      comp_vile,      cf_server, netcmd_comp_2) {}
CONSOLE_NETVAR(comp_pain,      comp_pain,      cf_server, netcmd_comp_3) {}
CONSOLE_NETVAR(comp_skull,     comp_skull,     cf_server, netcmd_comp_4) {}
CONSOLE_NETVAR(comp_blazing,   comp_blazing,   cf_server, netcmd_comp_5) {}
CONSOLE_NETVAR(comp_doorlight, comp_doorlight, cf_server, netcmd_comp_6) {}
CONSOLE_NETVAR(comp_model,     comp_model,     cf_server, netcmd_comp_7) {}
CONSOLE_NETVAR(comp_god,       comp_god,       cf_server, netcmd_comp_8) {}
CONSOLE_NETVAR(comp_falloff,   comp_falloff,   cf_server, netcmd_comp_9) {}
CONSOLE_NETVAR(comp_floors,    comp_floors,    cf_server, netcmd_comp_10) {}
CONSOLE_NETVAR(comp_skymap,    comp_skymap,    cf_server, netcmd_comp_11) {}
CONSOLE_NETVAR(comp_pursuit,   comp_pursuit,   cf_server, netcmd_comp_12) {}
CONSOLE_NETVAR(comp_doorstuck, comp_doorstuck, cf_server, netcmd_comp_13) {}
CONSOLE_NETVAR(comp_staylift,  comp_staylift,  cf_server, netcmd_comp_14) {}
CONSOLE_NETVAR(comp_zombie,    comp_zombie,    cf_server, netcmd_comp_15) {}
CONSOLE_NETVAR(comp_stairs,    comp_stairs,    cf_server, netcmd_comp_16) {}
CONSOLE_NETVAR(comp_infcheat,  comp_infcheat,  cf_server, netcmd_comp_17) {}
CONSOLE_NETVAR(comp_zerotags,  comp_zerotags,  cf_server, netcmd_comp_18) {}

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
  C_AddCommand(playdemo);
  C_AddCommand(timedemo);
  C_AddCommand(cooldemo);
  C_AddCommand(stopdemo);
  C_AddCommand(exitlevel);
  C_AddCommand(addfile);
  C_AddCommand(listwads);
  C_AddCommand(rngseed);
  C_AddCommand(kill);
  C_AddCommand(map);
  C_AddCommand(name);
  C_AddCommand(wipe_speed);
  C_AddCommand(textmode_startup);
  
  C_AddCommand(chatmacro0);
  C_AddCommand(chatmacro1);
  C_AddCommand(chatmacro2);
  C_AddCommand(chatmacro3);
  C_AddCommand(chatmacro4);
  C_AddCommand(chatmacro5);
  C_AddCommand(chatmacro6);
  C_AddCommand(chatmacro7);
  C_AddCommand(chatmacro8);
  C_AddCommand(chatmacro9);
  
  C_AddCommand(weappref_1);
  C_AddCommand(weappref_2);
  C_AddCommand(weappref_3);
  C_AddCommand(weappref_4);
  C_AddCommand(weappref_5);
  C_AddCommand(weappref_6);
  C_AddCommand(weappref_7);
  C_AddCommand(weappref_8);
  C_AddCommand(weappref_9);
  
  C_AddCommand(comp_telefrag);
  C_AddCommand(comp_dropoff);
  C_AddCommand(comp_vile);
  C_AddCommand(comp_pain);
  C_AddCommand(comp_skull);
  C_AddCommand(comp_blazing);
  C_AddCommand(comp_doorlight);
  C_AddCommand(comp_model);
  C_AddCommand(comp_god);
  C_AddCommand(comp_falloff);
  C_AddCommand(comp_floors);
  C_AddCommand(comp_skymap);
  C_AddCommand(comp_pursuit);
  C_AddCommand(comp_doorstuck);
  C_AddCommand(comp_staylift);
  C_AddCommand(comp_zombie);
  C_AddCommand(comp_stairs);
  C_AddCommand(comp_infcheat);
  C_AddCommand(comp_zerotags);
      
}
