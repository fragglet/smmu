/******************************* console **********************************/
                // Copyright(C) 1999 Simon Howard 'Fraggle' //
//
// Command list
//
// A list of all the console commands, and all the variables. This and 
// c_handle.c are what interface with the rest of the game
//
//      Long term aim: to replace this static list with a dynamic, more
//                   'legacy-style' system of scattering commands to their
//                   appropriate modules and having all parts of the game
//                   add their own commands at run-time. Not too difficult,
//                   I just have to get round to actually doing it.

//              module names added for when this actually happens.
//              a new g_?? module might be needed as there are a hell of
//              a lot of commands to go in g_game.c and it is big enough
//              already.


/* includes ************************/

#include <stdio.h>

#include "c_io.h"
#include "c_net.h"
#include "c_runcmd.h"
#include "c_handle.h"
#include "doomstat.h"

#include "d_main.h"
#include "g_game.h"
#include "hu_frags.h"
#include "hu_stuff.h"
#include "i_video.h"
#include "f_wipe.h"
#include "m_random.h"
#include "p_info.h"
#include "p_skin.h"
#include "p_chase.h"
#include "r_draw.h"
#include "r_main.h"
#include "r_sky.h"
#include "r_plane.h"
#include "r_things.h"
#include "s_sound.h"
#include "ser_main.h"
#include "t_script.h"
#include "version.h"

int default_psprites=1;
extern skill_t gameskill;
extern int swirly_water;

/***************************************************************************
                'defines': string values for variables
***************************************************************************/

char *yesno[]={"no","yes"};
char *onoff[]={"off","on"};

char *colournames[]= {"green","indigo","brown","red","pink","tomato","cream",
                      "white","dirt","blue","vomit", "black"};
char *textcolours[]={
        FC_BRICK  "brick" FC_RED,
        FC_TAN    "tan"   FC_RED,
        FC_GRAY   "gray"  FC_RED,
        FC_GREEN  "green" FC_RED,
        FC_BROWN  "brown" FC_RED,
        FC_GOLD   "gold"  FC_RED,
        FC_RED    "red"   FC_RED,
        FC_BLUE   "blue"  FC_RED,
        FC_ORANGE "orange"FC_RED,
        FC_YELLOW "yellow"FC_RED
};
char *skills[]=
{"im too young to die", "hey not too rough", "hurt me plenty",
 "ultra violence", "nightmare"};
char *bfgtypestr[3]= {"bfg9000", "classic", "bfg11k"};
char *dmstr[] = {"co-op", "dm", "altdeath", "trideath"};

// version hack

char *verdate_hack = (char*)version_date;
char *vername_hack = (char*)version_name;

/***************************************************************************
        Variables. All the variables are stored in an array of variable_t's
        called 'vars'. The actual commands in the 'commands' array point to
        entries in the 'vars' array.
 **************************************************************************/

enum
{
        var_skill, var_bfglook,
        var_conheight, var_conspeed,
        var_allowmlook, var_colour,
        var_name, var_bfgtype, 
        var_autoaim,
        var_recoil,
        var_nomonsters, var_fast, var_respawn,
        var_deathmatch, var_wipespeed,
        var_pushers, var_varfriction,
};

variable_t vars[]=
{
/***********************************************************************
Variable          Default               Type       Range, Defines
*************************************************************************/
{&gameskill,     &defaultskill,         vt_int,    0,4, skills},
{&bfglook,        NULL,                 vt_int,    0,2, NULL},
{&c_height,       NULL,                 vt_int,    20,200,NULL},
{&c_speed,        NULL,                 vt_int,    1,200,NULL},
{&allowmlook,     NULL,                 vt_int,    0,1, onoff},
{&default_colour, NULL,                 vt_int,    0,TRANSLATIONCOLOURS-1, colournames},
{&default_name,   NULL,                 vt_string, 0,18, NULL},
{&bfgtype,       &default_bfgtype,      vt_int,    0,2, bfgtypestr},
{&autoaim,       &default_autoaim,      vt_int,    0,1, onoff},
{&weapon_recoil, &default_weapon_recoil,vt_int,    0,1, onoff},
{&nomonsters,     NULL,                 vt_int,    0,1, onoff},
{&fastparm,       NULL,                 vt_int,    0,1, onoff},
{&respawnparm,    NULL,                 vt_int,    0,1, onoff},
{&deathmatch,     NULL,                 vt_int,    0,3, dmstr},
{&wipe_speed,     NULL,                 vt_int,    1,200,NULL},
{&allow_pushers, &default_allow_pushers,vt_int,    0,1, onoff},
{&variable_friction,&default_variable_friction,vt_int,    0,1, onoff},
};

/******** monster variables *********/

enum
{
        mon_remember, mon_infight, mon_backing, mon_avoid, mon_friction,
        mon_climb, mon_helpfriends, mon_distfriend
};

variable_t monst[] =
{
/***********************************************************************
Variable          Default               Type       Range, Defines       **/
{&monsters_remember,&default_monsters_remember, vt_int,    0,1,   onoff},
{&monster_infighting,&default_monster_infighting, vt_int,    0,1,   onoff},
{&monster_backing, &default_monster_backing, vt_int,    0,1,   onoff},
{&monster_avoid_hazards, &default_monster_avoid_hazards, vt_int,    0,1,   onoff},
{&monster_friction, &default_monster_friction, vt_int,    0,1,   onoff},
{&monkeys,       &default_monkeys,      vt_int,    0,1,   onoff},
{&help_friends,  &default_help_friends, vt_int,    0,1,   onoff},
{&distfriend,    &default_distfriend,  vt_int,    0,1024,NULL}
};

/*************************************************************************
        Constants. 'consts' is the same as the 'vars' list, but points to
        variables that cannot be changed (directly) by the player
 *************************************************************************/

enum
{
        const_version, const_creator, const_opt, const_rseed,
        const_verdate, const_vername
};

variable_t consts[]=
{
{&VERSION,      NULL,        vt_int},
{&info_creator, NULL,        vt_string},
{&cmdoptions,   NULL,        vt_string},
{&rngseed,      NULL,        vt_int},
{&verdate_hack, NULL,        vt_string},
{&vername_hack, NULL,        vt_string}
};

/*************************************************************************
                Commands.

  There is a seperate entry in this array for each console command that
  can be run in the game.

*************************************************************************/

command_t commands[]=
{
/***********************************************************************
Command Name    type        flags                 variable/handler
************************************************************************/

                // p_*  / g_cmd.c

{"colour",      ct_variable,cf_netvar|cf_handlerset, vars+var_colour,C_Playercolour, cmd_colour},
{"deathmatch",  ct_variable,cf_netvar|cf_server,  vars+var_deathmatch,NULL, cmd_deathmatch},
{"exitlevel",   ct_command, cf_server|cf_netvar|
                            cf_level,             NULL,C_ExitLevel, cmd_exitlevel},
{"fast",        ct_variable,cf_server|cf_netvar,  vars+var_fast,NULL, cmd_fast},
{"kill",        ct_command, cf_netvar|cf_level,   NULL,C_Kill, cmd_kill},
{"map",         ct_command, cf_server|cf_netvar,  NULL,C_Map, cmd_map},
{"name",        ct_variable,cf_netvar|cf_handlerset,vars+var_name,C_Playername,cmd_name},
{"nomonsters",  ct_variable,cf_server|cf_netvar,  vars+var_nomonsters,NULL,cmd_nomonsters},
{"respawn",     ct_variable,cf_server|cf_netvar,  vars+var_respawn,NULL,cmd_respawn},
{"skill",       ct_variable,cf_server|cf_netvar,  vars+var_skill,C_Skill,cmd_skill},
{"skin",        ct_command, cf_netvar,            NULL,P_ChangeSkin,cmd_skin},
{"sv_allowmlook",ct_variable,cf_netvar|cf_server, vars+var_allowmlook,NULL,cmd_allowmlook},
{"sv_autoaim",  ct_variable,cf_netvar|cf_server,  vars+var_autoaim, NULL,cmd_autoaim},
{"sv_bfglook",  ct_variable,cf_server|cf_netvar,  vars+var_bfglook,NULL,cmd_bfglook},
{"sv_bfgtype",  ct_variable,cf_netvar|cf_server,  vars+var_bfgtype,NULL,cmd_bfgtype},
{"sv_recoil",   ct_variable,cf_server|cf_netvar,  vars+var_recoil,NULL,cmd_recoil},
{"pushers",     ct_variable,cf_netvar|cf_server,  vars+var_pushers,NULL,cmd_pushers},
{"varfriction", ct_variable,cf_netvar|cf_server,  vars+var_varfriction,NULL,cmd_varfriction},

        // monster variables

{"mon_remember",ct_variable,cf_netvar|cf_server,  monst+mon_remember,NULL, cmd_monremember},
{"mon_infight", ct_variable,cf_netvar|cf_server,  monst+mon_infight,NULL, cmd_moninfight},
{"mon_backing", ct_variable,cf_netvar|cf_server,  monst+mon_backing,NULL, cmd_monbacking},
{"mon_avoid",   ct_variable,cf_netvar|cf_server,  monst+mon_avoid,NULL, cmd_monavoid},
{"mon_friction",ct_variable,cf_netvar|cf_server,  monst+mon_friction,NULL,cmd_monfriction},
{"mon_climb",   ct_variable,cf_netvar|cf_server,  monst+mon_climb,NULL, cmd_monclimb},
{"mon_helpfriends",ct_variable,cf_netvar|cf_server,monst+mon_helpfriends,NULL,cmd_monhelpfriends},
{"mon_distfriend",ct_variable,cf_netvar|cf_server, monst+mon_distfriend,NULL,cmd_mondistfriend},

{"creator",     ct_constant,0,                    consts+const_creator,NULL},

{"centremsg",   ct_command, 0,                    NULL,C_Centremsg},
{"playermsg",   ct_command, 0,                    NULL,C_Playermsg},
{"spawn",       ct_command, 0,                    NULL,C_Spawn},
{"linetrigger", ct_command, 0,                    NULL,C_LineTrigger},

                /** other commands **/
        // c_*.c

{"alias",       ct_command, 0,                    NULL,C_Alias},
{"cmdlist",     ct_command, 0,                    NULL,C_Cmdlist},
{"c_speed",     ct_variable,0,                    vars+var_conspeed,NULL},
{"c_height",    ct_variable,0,                    vars+var_conheight,NULL},
{"echo",        ct_command, 0,                    NULL,C_Echo},
{"delay",       ct_command, 0,                    NULL,C_Delay},
{"version",     ct_constant,0,                    consts+const_version,NULL},
{"ver_date",    ct_constant,0,                    consts+const_verdate,NULL},
{"ver_name",    ct_constant,0,                    consts+const_vername,NULL},
{"flood",       ct_command, 0,                    NULL,C_Flood},
{"opt",         ct_constant,0,                    consts+const_opt,NULL},

        // m_*
 
{"addfile",     ct_command, cf_notnet,            NULL,C_Addfile},
{"listwads",    ct_command, 0,                    NULL,D_ListWads},
{"wipe_speed",  ct_variable,0,                    vars+var_wipespeed,NULL},
{"rndseed",     ct_constant,0,                    consts+const_rseed,NULL},

                /** Constants **/

                /** special 'end' last command **/
{"end",ct_end}
};
