/************************* game console commands ***************************/
                   // copyright(c) 1999 Simon Howard //

// console commands controlling the game functions.

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "c_cmdlst.h"
#include "g_game.h"
#include "w_wad.h"

/****** externs ******/

extern int automlook;
extern int invert_mouse;
extern boolean sendpause;
extern int forwardmove[2];
extern int sidemove[2];

/****** variables *******/

int turbo_scale = 100;

variable_t var_automlook = 
{
        &automlook,      NULL,
        vt_int,
        0,1, onoff
};
variable_t var_bobbing = 
{
        &player_bobbing, NULL,
        vt_int,    0,1, onoff
};
variable_t var_invertmouse =
{
        &invert_mouse,   NULL,
        vt_int,    0,1, onoff
};
variable_t var_cooldemo =
{
        &cooldemo,       NULL,
        vt_int,    0,1, onoff
};
variable_t var_turbo =
{
        &turbo_scale,           NULL,
        vt_int,     10, 400,
};

/******* handler functions ********/

void G_Turbo()
{
      C_Printf ("turbo scale: %i%%\n",turbo_scale);
      forwardmove[0] = (0x19*turbo_scale)/100;
      forwardmove[1] = (0x32*turbo_scale)/100;
      sidemove[0] = (0x18*turbo_scale)/100;
      sidemove[1] = (0x28*turbo_scale)/100;
}

void G_Consolemode()
{
        C_SetConsole();
}

void G_Error()
{
        I_Error(c_args);
}

void G_QuitGame()
{
        exit(0);
}

void G_Pause()
{
        sendpause = true;
}

void G_CmdPlayDemo()
{
        if(W_CheckNumForName(c_argv[0]) == -1)
        {
                C_Printf("%s not found\n",c_argv[0]);
                return;
        }
        G_DeferedPlayDemo(c_argv[0]);
	singledemo = true;            // quit after one demo
}

void G_CmdTimeDemo()
{
        G_TimeDemo(c_argv[0]);
}

void G_CmdStopDemo()
{
        G_StopDemo();
}

void G_AnimShot()
{
        if(!c_argc)
        {
                C_Printf(
			"animated screenshot.\n"
                        "usage: animshot <frames>\n");
                return;
        }
        animscreenshot = atoi(c_argv[0]);
        consoleactive = 0;
}

/******* game command list *******/

command_t g_commands[] =
{
        {
                "i_error",     ct_command,
                0,
                NULL,G_Error
        },
        {
                "alwaysmlook", ct_variable,
                0,
                &var_automlook,NULL
        },
        {
                "bobbing",     ct_variable,
                0,
                &var_bobbing, NULL
        },
        {
                "starttitle",  ct_command,
                cf_notnet,
                NULL,D_StartTitle
        },
        {
                "endgame",     ct_command,
                cf_notnet,
                NULL,G_Consolemode
        },
        {
                "invertmouse", ct_variable,
                0,
                &var_invertmouse,NULL
        },
        {
                "pause",       ct_command,
                cf_server,
                NULL,G_Pause
        },
        {
                "quit",        ct_command,
                0,
                NULL,G_QuitGame
        },
        {
                "animshot",    ct_command,
                0,
                NULL,G_AnimShot
        },
        {
                "turbo",        ct_variable,
                0,
                &var_turbo, G_Turbo
        },

          /******* demo stuff *********/
        {
                "playdemo",    ct_command,
                cf_notnet,
                NULL,G_CmdPlayDemo
        },
        {
                "stopdemo",    ct_command,
                cf_notnet,
                NULL,G_CmdStopDemo
        },
        {
                "timedemo",    ct_command,
                cf_notnet,
                NULL,G_CmdTimeDemo
        },
        {
                "cooldemo",    ct_variable,
                0,
                &var_cooldemo,NULL
        },

        {"end", ct_end}
};

void G_AddCommands()
{
        C_AddCommandList(g_commands);
}
