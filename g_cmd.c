/************************* game console commands ***************************/
                   // copyright(c) 1999 Simon Howard //

// console commands controlling the game functions.

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "c_net.h"
#include "f_wipe.h"
#include "g_game.h"
#include "m_random.h"
#include "p_inter.h"
#include "w_wad.h"

/****** externs ******/

extern int automlook;
extern int invert_mouse;
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
     I_Quit();
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
CONSOLE_COMMAND(addfile, cf_notnet)
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

        // player skin
CONSOLE_NETCMD(skin, 0, netcmd_skin)
{
     P_ChangeSkin();
}

        // screen wipe speed
VARIABLE_INT(wipe_speed, NULL,                  1, 200, NULL);
CONSOLE_VARIABLE(wipe_speed, wipe_speed, 0) {}


void G_AddCommands()
{
      C_AddCommand(i_error);
      C_AddCommand(starttitle);
      C_AddCommand(endgame);
      C_AddCommand(pause);
      C_AddCommand(quit);
      C_AddCommand(animshot);
      C_AddCommand(alwaysmlook);
      C_AddCommand(bobbing);
      C_AddCommand(invertmouse);
      C_AddCommand(turbo);
      C_AddCommand(playdemo);
      C_AddCommand(timedemo);
      C_AddCommand(cooldemo);
      C_AddCommand(exitlevel);
      C_AddCommand(addfile);
      C_AddCommand(listwads);
      C_AddCommand(rngseed);
      C_AddCommand(kill);
      C_AddCommand(map);
      C_AddCommand(name);
      C_AddCommand(skin);
      C_AddCommand(wipe_speed);
}
