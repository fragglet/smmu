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

boolean default_psprites=1;
extern skill_t gameskill;
extern int automlook;
extern int invert_mouse;
extern int swirly_water;
extern int screenSize;

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
char *cross_str[]= {"none", "cross", "angle"};
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
        var_automlook, var_skill, var_bfglook,
        var_stretchsky, var_conheight, var_conspeed, var_comport,
        var_allowmlook, var_invertmouse, var_colour,
        var_name, var_psprites, var_lefthanded, var_bfgtype, var_crosshair,
        var_chasecam, var_planeview, var_swirlywater, var_autoaim,
        var_ticker, var_precache, var_s_precache, var_pitched,
        var_homflash, var_disk, var_recoil, var_screensize,
        var_nomonsters, var_fast, var_respawn,
        var_bobbing, var_deathmatch, var_wipespeed,
        var_blockmap, var_flatskip, var_vpo,
        var_pushers, var_varfriction, var_obcolour,
        var_obituaries, var_walkcam
};

variable_t vars[]=
{
/***********************************************************************
Variable            Type            Range                   Defines
*************************************************************************/
{&automlook,        vt_int,         0,1,                    onoff},
{&gameskill,        vt_int,         0,4,                    skills},
{&bfglook,          vt_int,         0,2,                    NULL},
{&stretchsky,       vt_int,         0,1,                    onoff},
{&c_height,         vt_int,         20,200,                 NULL},
{&c_speed,          vt_int,         1,200,                  NULL},
{&comport,          vt_int,         1,4,                    NULL},
{&allowmlook,       vt_int,         0,1,                    onoff},
{&invert_mouse,     vt_int,         0,1,                    onoff},
{&default_colour,   vt_int,         0,TRANSLATIONCOLOURS-1, colournames},
{&default_name,     vt_string,      0,18,                   NULL},
{&default_psprites, vt_int,         0,1,                    yesno},
{&lefthanded,       vt_int,         0,1,                    yesno},
{&bfgtype,          vt_int,         0,2,                    bfgtypestr},
{&crosshairnum,     vt_int,         0,CROSSHAIRS,           cross_str},
{&chasecam_active,  vt_int,         0,1,                    onoff},
{&visplane_view,    vt_int,         0,1,                    onoff},
{&swirly_water,     vt_int,         0,1,                    onoff},
{&autoaim,          vt_int,         0,1,                    onoff},
{&showticker,       vt_int,         0,1,                    onoff},
{&r_precache,       vt_int,         0,1,                    onoff},
{&s_precache,       vt_int,         0,1,                    onoff},
{&pitched_sounds,   vt_int,         0,1,                    onoff},
{&flashing_hom,     vt_int,         0,1,                    onoff},
{&disk_icon,        vt_int,         0,1,                    onoff},
{&weapon_recoil,    vt_int,         0,1,                    onoff},
{&screenSize,       vt_int,         0,8,                    NULL},
{&nomonsters,       vt_int,         0,1,                    onoff},
{&fastparm,         vt_int,         0,1,                    onoff},
{&respawnparm,      vt_int,         0,1,                    onoff},
{&player_bobbing,   vt_int,         0,1,                    onoff},
{&deathmatch,       vt_int,         0,3,                    dmstr},
{&wipe_speed,       vt_int,         1,200,                  NULL},
{&blockmapbuild,    vt_int,         0,1,                    onoff},
{&flatskip,         vt_int,         0,100,                  NULL},
{&show_vpo,         vt_int,         0,1,                    yesno},
{&allow_pushers,    vt_int,         0,1,                    onoff},
{&variable_friction,vt_int,         0,1,                    onoff},
{&death_colour,     vt_int,         0,CR_LIMIT-1,           textcolours},
{&obituaries,       vt_int,         0,1,                    onoff},
{&walkcam_active,   vt_int,         0,1,                    onoff}
};

/******** monster variables *********/

enum
{
        mon_remember, mon_infight, mon_backing, mon_avoid, mon_friction,
        mon_climb, mon_helpfriends, mon_distfriend
};

variable_t monst[] =
{
{&monsters_remember,vt_int,         0,1,                    onoff},
{&monster_infighting,vt_int,        0,1,                    onoff},
{&monster_backing,  vt_int,         0,1,                    onoff},
{&monster_avoid_hazards,vt_int,     0,1,                    onoff},
{&monster_friction, vt_int,         0,1,                    onoff},
{&monkeys,          vt_int,         0,1,                    onoff},
{&help_friends,     vt_int,         0,1,                    onoff},
{&distfriend,       vt_int,         0,1024,                 NULL}
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
{&VERSION,          vt_int,         -1,-1,                  NULL},
{&info_creator,     vt_string,      -1,-1,                  NULL},
{&cmdoptions,       vt_string,      -1,-1,                  NULL},
{&rngseed,          vt_int,         -1,-1,                  NULL},
{&verdate_hack,     vt_string,      -1,-1,                  NULL},
{&vername_hack,     vt_string,      -1,-1,                  NULL}
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

                /** network variables first **/

{"colour",      ct_variable,cf_netvar,            vars+var_colour,C_Playercolour, cmd_colour},
{"deathmatch",  ct_variable,cf_netvar|cf_server,  vars+var_deathmatch,NULL, cmd_deathmatch},
{"exitlevel",   ct_command, cf_server|cf_netvar|
                            cf_level,             NULL,C_ExitLevel, cmd_exitlevel},
{"fast",        ct_variable,cf_server|cf_netvar,  vars+var_fast,NULL, cmd_fast},
{"kill",        ct_command, cf_netvar|cf_level,   NULL,C_Kill, cmd_kill},
{"map",         ct_command, cf_server|cf_netvar,  NULL,C_Map, cmd_map},
{"name",        ct_variable,cf_netvar,            vars+var_name,C_Playername,cmd_name},
{"nomonsters",  ct_variable,cf_server|cf_netvar,  vars+var_nomonsters,NULL,cmd_nomonsters},
{"nuke",        ct_command, cf_server|cf_level|
                            cf_netvar,            NULL,C_CheatNuke, cmd_nuke},
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
                 //(internal)
{"kick",        ct_command, cf_server,            NULL,C_Kick},
{"_cm",         ct_command, 0,                    NULL,C__Chat, cmd_chat},

        // monster variables
{"mon_remember",ct_variable,cf_netvar|cf_server,  monst+mon_remember,NULL, cmd_monremember},
{"mon_infight", ct_variable,cf_netvar|cf_server,  monst+mon_infight,NULL, cmd_moninfight},
{"mon_backing", ct_variable,cf_netvar|cf_server,  monst+mon_backing,NULL, cmd_monbacking},
{"mon_avoid",   ct_variable,cf_netvar|cf_server,  monst+mon_avoid,NULL, cmd_monavoid},
{"mon_friction",ct_variable,cf_netvar|cf_server,  monst+mon_friction,NULL,cmd_monfriction},
{"mon_climb",   ct_variable,cf_netvar|cf_server,  monst+mon_climb,NULL, cmd_monclimb},
{"mon_helpfriends",ct_variable,cf_netvar|cf_server,monst+mon_helpfriends,NULL,cmd_monhelpfriends},
{"mon_distfriend",ct_variable,cf_netvar|cf_server, monst+mon_distfriend,NULL,cmd_mondistfriend},

                /** other commands **/
// console                      c_io.c

{"alias",       ct_command, 0,                    NULL,C_Alias},
{"cmdlist",     ct_command, 0,                    NULL,C_Cmdlist},
{"c_speed",     ct_variable,0,                    vars+var_conspeed,NULL},
{"c_height",    ct_variable,0,                    vars+var_conheight,NULL},
{"echo",        ct_command, 0,                    NULL,C_Echo},
{"delay",       ct_command, 0,                    NULL,C_Delay},

// game                         g_game.c / m_cheat.c

{"i_error",     ct_command, 0,                    NULL,C_Error},
{"alwaysmlook", ct_variable,0,                    vars+var_automlook,NULL},
{"bobbing",     ct_variable,0,                    vars+var_bobbing, NULL},
{"endgame",     ct_command, cf_notnet,            NULL,C_Consolemode},
{"frags",       ct_command, 0,                    NULL,HU_FragsDump},
{"god",         ct_command, cf_notnet|cf_hu_copy|
                            cf_level,             NULL,C_CheatGod},
{"invertmouse", ct_variable,0,                    vars+var_invertmouse,NULL},
{"listskins",   ct_command, 0,                    NULL,P_ListSkins},
{"noclip",      ct_command, cf_notnet|cf_hu_copy|
                            cf_level,             NULL,C_CheatNoClip},
{"pause",       ct_command, cf_server,            NULL,C_Pause},
{"obituaries",  ct_variable,0,                    vars+var_obituaries,NULL},
{"obcolour",    ct_variable,0,                    vars+var_obcolour, NULL},
{"quit",        ct_command, 0,                    NULL,C_QuitGame},

// scripting                    t_script.c

{"t_dump",      ct_command, 0,                    NULL,T_Dump},
{"t_run",       ct_command, 0,                    NULL,T_ConsRun},

// editing commands              g_game.c / replace with FraggleScript

        //  <Afterglow> CENTER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
{"centremsg",   ct_command, 0,                    NULL,C_Centremsg},
{"playermsg",   ct_command, 0,                    NULL,C_Playermsg},
{"spawn",       ct_command, 0,                    NULL,C_Spawn},
{"linetrigger", ct_command, 0,                    NULL,C_LineTrigger},

// demo stuff                    g_game.c

{"playdemo",    ct_command, cf_notnet,            NULL,C_PlayDemo},
{"stopdemo",    ct_command, cf_notnet,            NULL,C_StopDemo},
{"timedemo",    ct_command, cf_notnet,            NULL,C_TimeDemo},
{"starttitle",  ct_command, cf_notnet,            NULL,D_StartTitle},

// hud                           hu_stuff.c

{"crosshair",   ct_variable,0,                    vars+var_crosshair,HU_CrossHairConsole},
{"show_vpo",    ct_variable,0,                    vars+var_vpo, NULL},

// rendering                     r_main.c / p_chase.c

{"chasecam",    ct_variable,0,                    vars+var_chasecam,P_ToggleChasecam},
{"walkcam",     ct_variable,cf_notnet,            vars+var_walkcam,P_ToggleWalk},
{"lefthanded",  ct_variable,0,                    vars+var_lefthanded,NULL},
{"r_blockmap",  ct_variable,0,                    vars+var_blockmap, NULL},
{"r_flatskip",  ct_variable,0,                    vars+var_flatskip,NULL},
{"r_homflash",  ct_variable,0,                    vars+var_homflash,NULL},
{"r_planeview", ct_variable,0,                    vars+var_planeview,NULL},
{"r_precache",  ct_variable,0,                    vars+var_precache,NULL},
{"r_showgun",   ct_variable,0,                    vars+var_psprites,C_Psprites},
{"r_showhom",   ct_command, 0,                    NULL,C_HomDetect},
{"r_stretchsky",ct_variable,0,                    vars+var_stretchsky,NULL},
{"r_swirl",     ct_variable,0,                    vars+var_swirlywater,NULL},

// video                           v_video.c / new v_misc.c ? / g_game.c

{"animshot",    ct_command, 0,                    NULL,C_AnimShot},
{"i_diskicon",  ct_variable,0,                    vars+var_disk, NULL},
{"screensize",  ct_variable,0,                    vars+var_screensize, C_ScreenSize},
{"v_mode",      ct_command, 0,                    NULL,C_VidMode},
{"v_modelist",  ct_command, 0,                    NULL,C_VidModeList},
{"v_ticker",    ct_variable,0,                    vars+var_ticker, NULL},

// netgame                         d_net.c

{"disconnect",  ct_command, cf_netonly,           NULL,C_Disconnect},
{"playerinfo",  ct_command, 0,                    NULL,C_Players},
{"say",         ct_command, 0,                    NULL,C_Say},

// sound                           s_sound.c

{"s_pitched",   ct_variable,0,                    vars+var_pitched,NULL},
{"s_precache",  ct_variable,0,                    vars+var_s_precache,NULL},

// serial comms                    ser_main.c

{"answer",      ct_command, cf_notnet,            NULL,C_Answer},
{"com",         ct_variable,0,                    vars+var_comport,NULL},
{"dial",        ct_command, cf_notnet,            NULL,C_Dial},
{"nullmodem",   ct_command, cf_notnet,            NULL,C_NullModem},

// misc                            indiv. modules

{"addfile",     ct_command, cf_notnet,            NULL,C_Addfile},
{"listwads",    ct_command, 0,                    NULL,D_ListWads},
{"wipe_speed",  ct_variable,0,                    vars+var_wipespeed,NULL},
{"guitest",     ct_command, 0,                    NULL,C_GUITest},
{"flood",       ct_command, 0,                    NULL,C_Flood},

                /** Constants **/
{"creator",     ct_constant,0,                    consts+const_creator,NULL},
{"opt",         ct_constant,0,                    consts+const_opt,NULL},
{"rndseed",     ct_constant,0,                    consts+const_rseed,NULL},
{"version",     ct_constant,0,                    consts+const_version,NULL},
{"ver_date",    ct_constant,0,                    consts+const_verdate,NULL},
{"ver_name",    ct_constant,0,                    consts+const_vername,NULL},

//{"rdefs",       ct_command, 0,                    NULL,G_ReloadDefaults},

                /** special 'end' last command **/
{"end",         ct_end,     0,                    NULL,NULL}
};
