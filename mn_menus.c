// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Menus
//
// the actual menus: structs and handler functions (if any)
// console commands to activate each menu
//
// By Simon Howard
//
//-----------------------------------------------------------------------------

#include <stdarg.h>

#include "doomdef.h"
#include "doomstat.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "d_main.h"
#include "dstrings.h"
#include "g_game.h"
#include "hu_over.h"
#include "i_video.h"
#include "m_random.h"
#include "mn_engin.h"
#include "mn_misc.h"
#include "r_defs.h"
#include "r_draw.h"
#include "s_sound.h"
#include "w_wad.h"
#include "v_video.h"

// menus: all in this file (not really extern)
extern menu_t menu_newgame;
extern menu_t menu_main;
extern menu_t menu_episode;
extern menu_t menu_startmap;

// Blocky mode, has default, 0 = high, 1 = normal
//int     detailLevel;    obsolete -- killough
int screenSize;      // screen size

char *mn_phonenum;           // phone number to dial
char *mn_demoname;           // demo to play
char *mn_wadname;            // wad to load
char *mn_startlevel;           // level to start on in multiplayer

void MN_InitMenus()
{
  mn_phonenum = Z_Strdup("555-1212", PU_STATIC, 0);
  mn_demoname = Z_Strdup("demo1", PU_STATIC, 0);
  mn_wadname = Z_Strdup("", PU_STATIC, 0);
  mn_startlevel = Z_Strdup(gamemode == commercial ? "map01" : "e1m1",
			   PU_STATIC, 0);
}

/***************************** THE MENUS **********************************/

/***********************
              MAIN MENU
 ***********************/

void MN_MainMenuDrawer();

menu_t menu_main =
{
  {
    // 'doom' title drawn by the drawer
    
    {it_runcmd, "new game",             "mn_newgame",            "M_NGAME"},
    {it_runcmd, "options",              "mn_options",            "M_OPTION"},
    {it_runcmd, "load game",            "mn_loadgame",           "M_LOADG"},
    {it_runcmd, "save game",            "mn_savegame",           "M_SAVEG"},
    {it_runcmd, "features",             "mn_features",           "M_FEAT"},
    {it_runcmd, "quit",                 "mn_quit",               "M_QUITG"},
    {it_end},
  },
  100, 65,                // x, y offsets
  0,                     // start with 'new game' selected
  mf_skullmenu,          // a skull menu
  MN_MainMenuDrawer
};

void MN_MainMenuDrawer()
{
  // hack for m_doom compatibility
   V_DrawPatch(94, 2, 0, W_CacheLumpName("M_DOOM", PU_CACHE));
}

CONSOLE_COMMAND(mn_newgame, cf_notnet)
{
  if(gamemode == commercial)
    {
      // dont use new game menu if not needed
      if(!modifiedgame && gamemode==commercial
	 && W_CheckNumForName("START") >= 0
	 && use_startmap)
        {
	  if(use_startmap == -1)              // not asked yet
	    MN_StartMenu(&menu_startmap);
	  else
            {        // use start map 
	      G_DeferedInitNew(defaultskill, "START");
	      MN_ClearMenus ();
            }
        }
      else
	MN_StartMenu(&menu_newgame);
    }
  else
    {
      if(modifiedgame) MN_StartMenu(&menu_newgame);
      else
	{
	  // hack -- cut off thy flesh consumed if not retail
	  if(gamemode != retail)
	    menu_episode.menuitems[5].type = it_end;

	  MN_StartMenu(&menu_episode);
	}
    }
}

CONSOLE_COMMAND(mn_quit, 0)
{
  int quitmsgnum;
  char quitmsg[128];

  if(cmdtype != c_menu && menuactive) return;

  quitmsgnum = M_Random() % 14;
  sprintf(quitmsg, "%s\n\n" DOSY, endmsg[quitmsgnum]);
  
  MN_Question(quitmsg, "quit");

}

/************************
        SELECT EPISODE
 ************************/

int start_episode;

menu_t menu_episode =
{
  {
    {it_title, "which episode?",             NULL,           "M_EPISOD"},
    {it_gap},
    {it_runcmd, "knee deep in the dead",     "mn_episode 1",  "M_EPI1"},
    {it_runcmd, "the shores of hell",        "mn_episode 2",  "M_EPI2"},
    {it_runcmd, "inferno!",                  "mn_episode 3",  "M_EPI3"},
    {it_runcmd, "thy flesh consumed",        "mn_episode 4",  "M_EPI4"},
    {it_end},
  },
  40, 30,              // x, y offsets
  2,                   // select episode 1
  mf_skullmenu,        // skull menu
};

// console command to select episode

CONSOLE_COMMAND(mn_episode, cf_notnet)
{
  if(!c_argc)
    {
      C_Printf("usage: episode <epinum>\n");
      return;
    }

  start_episode = atoi(c_argv[0]);

  if(gamemode == shareware && start_episode > 1)
    {
      MN_Alert(SWSTRING);
      return;
    }

  MN_StartMenu(&menu_newgame);
}


/************************
           NEW GAME MENU
 ************************/

menu_t menu_newgame =
{
  {
    {it_title,  "new game",             NULL,                   "M_NEWG"},
    {it_gap},
    {it_info,   "choose skill level",   NULL,                   "M_SKILL"},
    {it_gap},
    {it_runcmd, "i'm too young to die", "newgame 0",            "M_JKILL"},
    {it_runcmd, "hey, not too rough",   "newgame 1",            "M_ROUGH"},
    {it_runcmd, "hurt me plenty.",      "newgame 2",            "M_HURT"},
    {it_runcmd, "ultra-violence",       "newgame 3",            "M_ULTRA"},
    {it_runcmd, "nightmare!",           "newgame 4",            "M_NMARE"},
    {it_end},
  },
  40, 15,               // x,y offsets
  6,                    // starting item: hurt me plenty
  mf_skullmenu,         // is a skull menu
};

CONSOLE_COMMAND(newgame, cf_notnet)
{
  int skill = gameskill;
  
  // skill level is argv 0
  
  if(c_argc) skill = atoi(c_argv[0]);

  if(gamemode == commercial || modifiedgame)
    {
      // start on newest level from wad
      G_DeferedInitNew(skill, startlevel);
    }
  else
    {
      // start on first level of selected episode
      G_DeferedInitNewNum(skill, start_episode, 1);
    }

  MN_ClearMenus();
}

menu_t menu_startmap =
{
  {
    {it_title,  "new game",             NULL,                   "M_NEWG"},
    {it_gap},
    {it_info,   "SMMU includes a 'start map' to let"},
    {it_info,   "you start new games from in a level."},
    {it_info,   "would you like to use this or the"},
    {it_info,   "menu in the future to start new"},
    {it_info,   "games ?\n"},
    {it_gap},
    {it_runcmd, "use the start map",            "use_startmap 1; mn_newgame"},
    {it_runcmd, "use the menu",                 "use_startmap 0; mn_newgame"},
    {it_end},
  },
  40, 15,               // x,y offsets
  8,                    // starting item: hurt me plenty
  mf_leftaligned | mf_background, 
};

char *str_startmap[] = {"ask", "no", "yes"};
VARIABLE_INT(use_startmap, NULL, -1, 1, str_startmap);
CONSOLE_VARIABLE(use_startmap, use_startmap, 0) {}

/***********************
          FEATURES MENU
 ***********************/

menu_t menu_features =
{
  {
    {it_title,  FC_GOLD "features",     NULL,                   "M_FEAT"},
    {it_gap},
    {it_gap},
    {it_runcmd, "multiplayer",          "mn_multi",              "M_MULTI"},
    {it_gap},
    {it_runcmd, "load wad",             "mn_loadwad",            "M_WAD"},
    {it_gap},
    {it_runcmd, "demos",                "mn_demos",              "M_DEMOS"},
    {it_gap},
    {it_runcmd, "about",                "mn_about",              "M_ABOUT"},
    {it_end},
  },
  100, 15,                              // x,y
  3,                                    // start item
  mf_leftaligned | mf_skullmenu         // skull menu
};

CONSOLE_COMMAND(mn_features, 0)
{
  MN_StartMenu(&menu_features);
}

/********************
               DEMOS
 ********************/

menu_t menu_demos = 
{
  {
    {it_title,      FC_GOLD "demos",          NULL,             "m_demos"},
    {it_gap},
    {it_info,       FC_GOLD "play demo"},
    {it_variable,   "demo name",              "mn_demoname"},
    {it_gap},
    {it_runcmd,     "play demo",              "mn_clearmenus; playdemo %mn_demoname"},
    {it_runcmd,     "time demo",              "mn_clearmenus; timedemo %mn_demoname"},
    {it_runcmd,     "stop playing demo",      "mn_clearmenus; stopdemo"},
    {it_gap},
    {it_info,       FC_GOLD "cameras"},
    {it_toggle,     "viewpoint changes",      "cooldemo"},
    {it_toggle,     "chasecam",               "chasecam"},
    {it_toggle,     "walkcam",                "walkcam"},
    {it_gap},
    {it_end},
  },
  150, 40,           // x,y
  3,                // start item
  mf_background,    // full screen
};

VARIABLE_STRING(mn_demoname,     NULL,           8);
CONSOLE_VARIABLE(mn_demoname,    mn_demoname,     0) {}

CONSOLE_COMMAND(mn_demos, cf_notnet)
{
  MN_StartMenu(&menu_demos);
}

/************************
        LOAD WAD
*************************/

menu_t menu_loadwad =
{
  {
    {it_title,     FC_GOLD "load wad",    NULL,                   "M_WAD"},
    {it_gap},
    {it_info,      FC_GOLD "load wad"},
    {it_variable,  "wad name",          "mn_wadname"},
    {it_runcmd,    "select wad",        "mn_selectwad"},
    {it_gap},
    {it_runcmd,    "load wad",          "addfile %mn_wadname; starttitle"},
    {it_end},
  },
  150, 40,                     // x,y offsets
  3,                          // starting item
  mf_background               // full screen 
};

VARIABLE_STRING(mn_wadname,     NULL,           15);
CONSOLE_VARIABLE(mn_wadname,    mn_wadname,     0) {}

CONSOLE_COMMAND(mn_loadwad, cf_notnet)
{
  MN_StartMenu(&menu_loadwad);
}

/************************
        MULTIPLAYER
 ************************/

menu_t menu_multiplayer =
{
  {
    {it_title,  FC_GOLD "multiplayer",  NULL,                   "M_MULTI"},
    {it_gap},
    {it_gap},
    {it_info,   FC_GOLD "connect:"},
    {it_runcmd, "serial/modem",         "mn_serial"},
    {it_runcmd, "tcp/ip",               "mn_tcpip"},
    {it_gap},
    {it_runcmd, "disconnect",           "disconnect"},
    {it_gap},
    {it_info,   FC_GOLD "setup"},
    {it_runcmd, "chat macros",          "mn_chatmacros"},
    {it_runcmd, "player setup",         "mn_player"},
    {it_runcmd, "game settings",        "mn_multigame"},
    {it_end},
  },
  100, 15,                                      // x,y offsets
  4,                                            // starting item
  mf_background|mf_leftaligned,                 // fullscreen
};

CONSOLE_COMMAND(mn_multi, 0)
{
  MN_StartMenu(&menu_multiplayer);
}

/************************
        GAME SETTINGS
 ************************/

enum
{
  cn_menu_setupgame,
  cn_serial_answer,
  cn_serial_connect,
  // no dial: answer is server so it sets the settings
  // dial is started immediately
  cn_udp_server,
} connect_type;

menu_t menu_multigame =
{
  {
    {it_title,    FC_GOLD "multiplayer",        NULL,             "M_MULTI"},
    {it_gap},
    {it_runcmd,   "done",                       "mn_startgame"},
    {it_gap},
    {it_info,     FC_GOLD "game settings"},
    {it_toggle,   "game type",                  "deathmatch"},
    {it_variable, "starting level",             "mn_startlevel"},
    {it_toggle,   "skill level",                "skill"},
    {it_toggle,   "no monsters",                "nomonsters"},
    {it_gap},
    {it_info,     FC_GOLD "auto-exit"},
    {it_variable, "time limit",                 "timelimit"},
    {it_variable, "frag limit",                 "fraglimit"},
    {it_gap},
    {it_runcmd,   "advanced..",                 "mn_advanced"},
    {it_end},
  },
  130, 15,
  2,                            // start
  mf_background,                // full screen
};

        // level to start on
VARIABLE_STRING(mn_startlevel,    NULL,   8);
CONSOLE_VARIABLE(mn_startlevel, mn_startlevel, cf_handlerset)
{
  char *newvalue = c_argv[0];

  // check for a valid level
  if(W_CheckNumForName(newvalue) == -1)
    MN_ErrorMsg("level not found!");
  else
    {
      Z_Free(mn_startlevel);
      mn_startlevel = Z_Strdup(newvalue, PU_STATIC, 0);
    }
}

CONSOLE_COMMAND(mn_multigame, 0)            // just setting options from menu
{
  connect_type = cn_menu_setupgame;
  MN_StartMenu(&menu_multigame);
}

CONSOLE_COMMAND(mn_ser_answer, 0)           // serial wait-for-call
{
  C_SetConsole();               // dont want demos interfering
  connect_type = cn_serial_answer;
  MN_StartMenu(&menu_multigame);
}

CONSOLE_COMMAND(mn_ser_connect, 0)          // serial nullmodem
{
  C_SetConsole();               // dont want demos interfering
  connect_type = cn_serial_connect;
  MN_StartMenu(&menu_multigame);
}

CONSOLE_COMMAND(mn_udpserv, 0)              // udp start server
{
  C_SetConsole();               // dont want demos interfering
  connect_type = cn_udp_server;
  MN_StartMenu(&menu_multigame);
}
        // start game
CONSOLE_COMMAND(mn_startgame, 0)
{
  char *console_cmds[] =
  {
    "mn_prevmenu",          // menu game setup
    "answer",               // cn_serial_answer
    "nullmodem",            // cn_serial_connect
    "connect",              // udp connect
  };
  
  cmdtype = c_menu;
  C_RunTextCmd(console_cmds[connect_type]);
}

/************************
    ADVANCED MULTIPLAYER
 ************************/

menu_t menu_advanced =
{
  {
    {it_title,    FC_GOLD "advanced",           NULL,             "M_MULTI"},
    {it_gap},
    {it_runcmd,   "done",                       "mn_prevmenu"},
    {it_gap},
    {it_toggle,   "fast monsters",              "fast"},
    {it_toggle,   "respawning monsters",        "respawn"},
    {it_gap},
    {it_toggle,   "allow mlook",                "allowmlook"},
    {it_toggle,   "allow mlook with bfg",       "bfglook"},
    {it_toggle,   "allow autoaim",              "autoaim"},
    {it_variable, "weapon change time",         "weapspeed"},
    {it_gap},
    {it_toggle,   "variable friction",          "varfriction"},
    {it_toggle,   "boom pusher objects",        "pushers"},
    {it_toggle,   "hurting floors(slime)",      "nukage"},
    {it_end},
  },
  170, 15,
  2,                            // start
  mf_background,                // full screen
};

CONSOLE_COMMAND(mn_advanced, cf_server)
{
  MN_StartMenu(&menu_advanced);
}

/************************
        TCP/IP MENU
 ************************/

menu_t menu_tcpip =
{
  {
    {it_title,  FC_GOLD "TCP/IP",            NULL,           "M_TCPIP"},
    {it_gap},
    {it_info,   "not implemented yet. :)"},
    {it_runcmd, "",                          "mn_prevmenu"},
    {it_end},
  },
  180,15,                       // x,y offset
  3,
  mf_background,                // full-screen
};

CONSOLE_COMMAND(mn_tcpip, 0)
{
  MN_StartMenu(&menu_tcpip);
}

/**************************
        SERIAL/MODEM MENU
 **************************/

menu_t menu_serial =
{
  {
    {it_title,  FC_GOLD "Serial/modem",          NULL,           "M_SERIAL"},
    {it_gap},
    {it_info,           FC_GOLD "settings"},
    {it_toggle,         "com port to use",      "com"},
    {it_variable,       "phone number",         "mn_phonenum"},
    {it_gap},
    {it_info,           FC_GOLD "connect:"},
    {it_runcmd,         "null modem link",      "mn_ser_connect"},
    {it_runcmd,         "dial",                 "dial %mn_phonenum"},
    {it_runcmd,         "wait for call",        "mn_ser_answer"},
    {it_end},
  },
  180,15,                       // x,y offset
  3,
  mf_background,                // fullscreen
};

CONSOLE_COMMAND(mn_serial, 0)
{
  MN_StartMenu(&menu_serial);
}

VARIABLE_STRING(mn_phonenum,     NULL,           126);
CONSOLE_VARIABLE(mn_phonenum,    mn_phonenum,     0) {}

/************************
        CHAT MACROS MENU
 ************************/

menu_t menu_chatmacros =
{
  {
    {it_title,  FC_GOLD "chat macros",           NULL,           "M_CHATM"},
    {it_gap},
    {it_variable,       "0",            "chatmacro0"},
    {it_variable,       "1",            "chatmacro1"},
    {it_variable,       "2",            "chatmacro2"},
    {it_variable,       "3",            "chatmacro3"},
    {it_variable,       "4",            "chatmacro4"},
    {it_variable,       "5",            "chatmacro5"},
    {it_variable,       "6",            "chatmacro6"},
    {it_variable,       "7",            "chatmacro7"},
    {it_variable,       "8",            "chatmacro8"},
    {it_variable,       "9",            "chatmacro9"},
    {it_end}
  },
  20,5,                                 // x, y offset
  2,                                    // chatmacro0 at start
  mf_background,                        // full-screen
};

CONSOLE_COMMAND(mn_chatmacros, 0)
{
  MN_StartMenu(&menu_chatmacros);
}

/************************
        PLAYER SETUP
 ************************/

void MN_PlayerDrawer();

menu_t menu_player =
{
  {
    {it_title,  FC_GOLD "player setup",           NULL,           "M_PLAYER"},
    {it_gap},
    {it_variable,       "player name",          "name"},
    {it_toggle,         "player colour",        "colour"},
    {it_toggle,         "player skin",          "skin"},
    {it_gap},
    {it_toggle,       "handedness",           "lefthanded"},
    {it_end}
  },
  150,5,                                // x, y offset
  2,                                    // chatmacro0 at start
  mf_background,                        // full-screen
  MN_PlayerDrawer
};

#define SPRITEBOX_X 200
#define SPRITEBOX_Y 80

void MN_PlayerDrawer()
{
  int lump;
  spritedef_t *sprdef;
  spriteframe_t *sprframe;
  patch_t *patch;
  
  V_DrawBox(SPRITEBOX_X, SPRITEBOX_Y, 80, 80);
  
  sprdef = &sprites[players[consoleplayer].skin->sprite];
  
  sprframe = &sprdef->spriteframes[0];
  lump = sprframe->lump[1];
  
  patch = W_CacheLumpNum(lump + firstspritelump, PU_CACHE);
  
  V_DrawPatchTranslated
    (
     SPRITEBOX_X + 40,
     SPRITEBOX_Y + 70,
     0,
     patch,
     players[displayplayer].colormap ?
           (char*)translationtables + 256*(players[displayplayer].colormap-1) :
           cr_red,
     -1
     );
}

CONSOLE_COMMAND(mn_player, 0)
{
  MN_StartMenu(&menu_player);
}

/************************
        OPTIONS MENU
 ************************/

menu_t menu_options =
{
  {
    {it_title,  FC_GOLD "options",              NULL,             "M_OPTTTL"},
    {it_gap},
    {it_info,   FC_GOLD "input"},
    {it_info,   FC_BRICK "key bindings",        "mn_keybindings"},
    {it_runcmd, "mouse options",                "mn_mouse"},
    {it_gap},
    {it_info,   FC_GOLD "output"},
    {it_runcmd, "video options",                "mn_video"},
    {it_runcmd, "sound options",                "mn_sound"},
    {it_gap},
    {it_info,   FC_GOLD "game options"},
    {it_runcmd, "compatibility",                "mn_compat"},
    {it_runcmd, "enemies",                      "mn_enemies"},
    {it_runcmd, "weapons",                      "mn_weapons"},
    {it_runcmd, "end game",                     "mn_endgame"},
    {it_gap},
    {it_info,   FC_GOLD "game widgets"},
    {it_runcmd, "hud settings",                 "mn_hud"},
    {it_runcmd, "status bar",                   "mn_status"},
    {it_runcmd, "automap",                      "mn_automap"},
    {it_end},
  },
  100, 15,                              // x,y offsets
  4,                                    // starting item: first selectable
  mf_background|mf_leftaligned,         // draw background: not a skull menu
};

CONSOLE_COMMAND(mn_options, 0)
{
  MN_StartMenu(&menu_options);
}

CONSOLE_COMMAND(mn_endgame, 0)
{
  if(gamestate == GS_DEMOSCREEN) return;
  if(cmdtype != c_menu && menuactive) return;
  
  MN_Question("do you want to end the game?\n\n"
	      "press y or n.",
	      "starttitle");
}

/*********************
        VIDEO MODES
 *********************/

void MN_VideoModeDrawer();

menu_t menu_video =
{
  {
    {it_title,        FC_GOLD "video",                NULL, "m_video"},
    {it_gap},
    {it_info,         FC_GOLD "mode"},
    {it_toggle,       "video mode",                   "v_mode"},
    {it_info,         "(vid mode here)"},
    {it_toggle,       "wait for retrace",             "v_retrace"},
    {it_runcmd,       "test framerate..",             "timedemo demo2; mn_clearmenus"},
    {it_slider,       "gamma correction",             "gamma"},
    
    {it_gap},
    {it_info,         FC_GOLD "rendering"},
    {it_slider,       "screen size",                  "screensize"},
    {it_toggle,       "hom detector flashes",         "r_homflash"},
    {it_toggle,       "translucency",                 "r_trans"},
    {it_variable,     "translucency percentage",      "r_tranpct"},
    
    {it_gap},
    {it_info,         FC_GOLD "misc."},
    {it_toggle,       "\"loading\" disk icon",        "v_diskicon"},
    {it_toggle,       "screenshot format",            "shot_type"},
    {it_toggle,       "text mode startup",            "textmode_startup"},
    
    {it_end},
  },
  200, 15,              // x,y offset
  3,                    // start on first selectable
  mf_background,        // full-screen menu
  MN_VideoModeDrawer
};

void MN_VideoModeDrawer()
{
  int lump;
  patch_t *patch;
  spritedef_t *sprdef;
  spriteframe_t *sprframe;

  menu_video.menuitems[4].description = videomodes[v_mode].description;
    
  // draw an imp fireball
  
  sprdef = &sprites[states[S_TBALL1].sprite];
  sprframe = &sprdef->spriteframes[0];
  lump = sprframe->lump[0];

  patch = W_CacheLumpNum(lump + firstspritelump, PU_CACHE);
  
  V_DrawBox(270, 110, 20, 20);
  V_DrawPatchTL(282, 122, 0, patch, NULL);
}

CONSOLE_COMMAND(mn_video, 0)
{
  MN_StartMenu(&menu_video);
}

/***********************
                SOUND
 ***********************/

menu_t menu_sound =
{
  {
    {it_title,      FC_GOLD "sound",                NULL, "m_sound"},
    {it_gap},
    {it_info,       FC_GOLD "volume"},
    {it_slider,     "sfx volume",                   "sfx_volume"},
    {it_slider,     "music volume",                 "music_volume"},
    {it_gap},
    {it_info,       FC_GOLD "setup"},
    {it_toggle,     "sound card",                   "snd_card"},
    {it_toggle,     "music card",                   "mus_card"},
    {it_toggle,     "autodetect voices",            "detect_voices"},
    {it_toggle,     "sound channels",               "snd_channels"},
    {it_gap},
    {it_info,       FC_GOLD "misc"},
    {it_toggle,     "precache sounds",              "s_precache"},
    {it_toggle,     "pitched sounds",               "s_pitched"},
    {it_end},
  },
  180, 15,                     // x, y offset
  3,                           // first selectable
  mf_background,               // full-screen menu
};

CONSOLE_COMMAND(mn_sound, 0)
{
  MN_StartMenu(&menu_sound);
}

/***********************
                MOUSE
 ***********************/

menu_t menu_mouse =
{
  {
    {it_title,      FC_GOLD "mouse",                NULL,   "m_mouse"},
    {it_gap},
    {it_toggle,     "enable mouse",                 "use_mouse"},
    {it_gap},
    {it_info,       FC_GOLD "sensitivity"},
    {it_slider,     "horizontal",                   "sens_horiz"},
    {it_slider,     "vertical",                     "sens_vert"},
    {it_gap},
    {it_info,       FC_GOLD "misc."},
    {it_toggle,     "invert mouse",                 "invertmouse"},
    {it_toggle,     "always mouselook",             "alwaysmlook"},
    {it_toggle,     "enable joystick",              "use_joystick"},
    {it_toggle,     "stretch sky for mlook",        "r_stretchsky"},

    {it_end},
  },
  200, 15,                      // x, y offset
  2,                            // first selectable
  mf_background,                // full-screen menu
};

CONSOLE_COMMAND(mn_mouse, 0)
{
  MN_StartMenu(&menu_mouse);
}

/************************
          HUD SETTINGS
 ************************/

menu_t menu_hud =
{
  {
    {it_title,      FC_GOLD "hud settings",         NULL,      "m_hud"},
    {it_gap},
    {it_info,       FC_GOLD "hud messages"},
    {it_toggle,     "messages",                     "messages"},
    {it_toggle,     "message colour",               "mess_colour"},
    {it_toggle,     "messages scroll",              "mess_scrollup"},
    {it_toggle,     "message lines",                "mess_lines"},
    {it_variable,   "message time (ms)",            "mess_timer"},
    {it_toggle,     "obituaries",                   "obituaries"},
    {it_toggle,     "obituary colour",              "obcolour"},
    {it_gap},
    {it_info,       FC_GOLD "full screen display"},
    {it_toggle,     "display type",                 "hu_overlay"},
    {it_toggle,     "hide secrets",                 "hu_hidesecrets"},
    {it_gap},
    {it_info,       FC_GOLD "misc."},
    {it_toggle,     "crosshair type",               "crosshair"},
    {it_toggle,     "show frags in DM",             "show_scores"},
    {it_end},
  },
  200, 15,                             // x,y offset
  3,
  mf_background,
};

CONSOLE_COMMAND(mn_hud, 0)
{
  MN_StartMenu(&menu_hud);
}

/***************************
        STATUS BAR SETTINGS
 ***************************/

menu_t menu_statusbar =
{
  {
    {it_title,      FC_GOLD "status bar",           NULL,           "m_stat"},
    {it_gap},
    {it_toggle,     "numbers always red",           "st_rednum"},
    {it_toggle,     "percent sign grey",            "st_graypct"},
    {it_toggle,     "single key display",           "st_singlekey"},
    {it_gap},
    {it_info,       FC_GOLD "status bar colours"},
    {it_variable,   "ammo ok percentage",           "ammo_yellow"},
    {it_variable,   "ammo low percentage",          "ammo_red"},
    {it_gap},
    {it_variable,   "armour high percentage",       "armor_green"},
    {it_variable,   "armour ok percentage",         "armor_yellow"},
    {it_variable,   "armour low percentage",        "armor_red"},
    {it_gap},
    {it_variable,   "health high percentage",       "health_green"},
    {it_variable,   "health ok percentage",         "health_yellow"},
    {it_variable,   "health low percentage",        "health_red"},
    {it_end},
  },
  200, 15,
  3,
  mf_background,
};

CONSOLE_COMMAND(mn_status, 0)
{
  MN_StartMenu(&menu_statusbar);
}

/************************
                AUTOMAP
 ************************/

menu_t menu_automap = 
{
  {
    {it_title,    FC_GOLD,                        NULL,         "m_auto"},
    {it_gap},
    {it_automap,  "background colour",            "mapcolor_back"},
    {it_automap,  "walls",                        "mapcolor_wall"},
    {it_automap,  "closed door",                  "mapcolor_clsd"},
    {it_automap,  "change in floor height",       "mapcolor_fchg"},
    {it_automap,  "red door",                     "mapcolor_rdor"},
    {it_automap,  "yellow door",                  "mapcolor_ydor"},
    {it_automap,  "blue door",                    "mapcolor_bdor"},
    {it_automap,  "teleport line",                "mapcolor_tele"},
    {it_automap,  "secret",                       "mapcolor_secr"},
    {it_automap,  "exit",                         "mapcolor_exit"},
    {it_automap,  "unseen line",                  "mapcolor_unsn"},

    {it_automap,  "sprite",                       "mapcolor_sprt"},
    {it_automap,  "crosshair",                    "mapcolor_hair"},
    {it_automap,  "single player arrow",          "mapcolor_sngl"},
    {it_automap,  "friend",                       "mapcolor_frnd"},
    {it_automap,  "red key",                      "mapcolor_rkey"},
    {it_automap,  "yellow key",                   "mapcolor_ykey"},
    {it_automap,  "blue key",                     "mapcolor_bkey"},

    {it_end},
  },
  200, 15,              // x,y
  2,                    // starting item
  mf_background,        // fullscreen
};

CONSOLE_COMMAND(mn_automap, 0)
{
  MN_StartMenu(&menu_automap);
}

/************************
                WEAPONS
 ************************/

menu_t menu_weapons =
{
  {
    {it_title,      FC_GOLD "weapons",              NULL,        "m_weap"},
    {it_gap},
    {it_info,       FC_GOLD "weapon options"},
    {it_toggle,     "bfg type",                       "bfgtype"},
    {it_toggle,     "bobbing",                        "bobbing"},
    {it_toggle,     "recoil",                         "recoil"},
    {it_info,       FC_BRICK "fist/chainsaw switch"},
    {it_gap},
    {it_info,       FC_GOLD "weapon prefs."},
    {it_variable,   "1st choice",                     "weappref_1"},
    {it_variable,   "2nd choice",                     "weappref_2"},
    {it_variable,   "3rd choice",                     "weappref_3"},
    {it_variable,   "4th choice",                     "weappref_4"},
    {it_variable,   "5th choice",                     "weappref_5"},
    {it_variable,   "6th choice",                     "weappref_6"},
    {it_variable,   "7th choice",                     "weappref_7"},
    {it_variable,   "8th choice",                     "weappref_8"},
    {it_variable,   "9th choice",                     "weappref_9"},
    {it_end},
  },
  150, 15,                             // x,y offset
  3,                                   // starting item
  mf_background,                       // full screen
};

CONSOLE_COMMAND(mn_weapons, 0)
{
  MN_StartMenu(&menu_weapons);
}

/**************************
            COMPATIBILITY
 **************************/

menu_t menu_compat =
{
  {
    {it_title,      FC_GOLD "compatibility",        NULL,        "m_compat"},
    {it_gap},
    {it_toggle,   "use start map",                          "use_startmap"},
    
    {it_toggle,   "some objects don't hang over cliffs",    "comp_dropoff"},
    {it_toggle,   "torque simulation disabled",             "comp_falloff"},
    
    {it_toggle,   "god mode isn't absolute",                "comp_god"},
    {it_toggle,   "power-up cheats have limited duration",  "comp_infcheat"},
    
    {it_toggle,   "sky unaffected by invulnerability",      "comp_skymap"},
    
    {it_toggle,   "blazing doors, double closing sound",    "comp_blazing"},
    {it_toggle,   "tagged door lighting effect off",        "comp_doorlight"},
    
    {it_toggle,   "pain elemental 20 lost soul limit",      "comp_pain"},
    {it_toggle,   "lost souls get stuck behind walls",      "comp_skull"},
    {it_toggle,   "monsters walk off lifts",                "comp_staylift"},
    {it_toggle,   "monsters get stuck to doortracks",       "comp_doorstuck"},
    {it_toggle,   "monsters don't give up pursuit",         "comp_pursuit"},
    {it_toggle,   "any monster can telefrag on map30",      "comp_telefrag"},
    {it_toggle,   "arch-vile resurrects invincible ghosts", "comp_vile"},
    
    {it_toggle,   "zombie players can exit levels",         "comp_zombie"},
    {it_toggle,   "use doom's stairbuilding method",        "comp_stairs"},
    {it_toggle,   "use doom's floor motion behaviour",      "comp_floors"},
    {it_toggle,   "use doom's linedef trigger model",       "comp_model"},
    {it_toggle,   "linedef effects with sector tag = 0",    "comp_zerotags"},
    {it_end},
  },
  270, 5,                     // x,y
  3,                           // starting item
  mf_background,               // full screen
};

CONSOLE_COMMAND(mn_compat, 0)
{
  MN_StartMenu(&menu_compat);
}

/**************************
                  ENEMIES
 **************************/

menu_t menu_enemies =
{
  {
    {it_title,      FC_GOLD "enemies",              NULL,      "m_enem"},
    {it_gap},
    {it_info,       FC_GOLD "monster options"},
    {it_toggle,     "monsters remember target",     "mon_remember"},
    {it_toggle,     "monster infighting",           "mon_infight"},
    {it_toggle,     "monsters back out",            "mon_backing"},
    {it_toggle,     "monsters avoid hazards",       "mon_avoid"},
    {it_toggle,     "affected by friction",         "mon_friction"},
    {it_toggle,     "climb tall stairs",            "mon_climb"},
    {it_gap},
    {it_info,       FC_GOLD "mbf friend options"},
    {it_variable,   "friend distance",              "mon_distfriend"},
    {it_toggle,     "rescue dying friends",         "mon_helpfriends"},
    {it_end},
  },
  200,15,                             // x,y offset
  3,                                  // starting item
  mf_background                       // full screen
};

CONSOLE_COMMAND(mn_enemies, 0)
{
  MN_StartMenu(&menu_enemies);
}

// sf: special framerate menu to show how your framerate compares
//     with _mine_

int framerates[] = {2462, 1870, 2460, 698, 489};
int this_framerate;
void MN_FrameRateDrawer();

menu_t menu_framerate = 
{
  {
    {it_title,    FC_GOLD "framerate"},
    {it_gap},
    {it_info,     "this graph shows your framerate against that"},
    {it_info,     "of a fast modern computer (using the same"},
    {it_info,     "vidmode)"},
    {it_gap},
    {it_runcmd,   "ok",          "mn_prevmenu"},
    {it_gap},
    {it_end},
  },
  15, 15,                                // x, y
  6,                                     // starting item
  mf_background | mf_leftaligned,        // align left
  MN_FrameRateDrawer,
};

#define BARCOLOR 208

void MN_FrameRateDrawer()
{
  int x, y;
  int scrwidth = SCREENWIDTH << hires;
  int linelength;
  char tempstr[50];
  
  // fast computers framerate is always 3/4 of screen

  sprintf(tempstr, "your computer: %i.%i fps",
	  this_framerate/10, this_framerate%10);
  MN_WriteText(tempstr, 50, 80);
  
  y = 93 << hires;
  linelength = (3 * scrwidth * this_framerate) / (4 * framerates[v_mode]);
  if(linelength > scrwidth) linelength = scrwidth-2;
 
  // draw your computers framerate
  for(x=0; x<linelength; x++)
    {
      *(screens[0] + y*scrwidth + x) = BARCOLOR;
      if(hires)
	*(screens[0] + (y+1)*scrwidth + x) = BARCOLOR;
    }

  sprintf(tempstr, "fast computer (k6-2 450): %i.%i fps",
	  framerates[v_mode]/10, framerates[v_mode]%10);
  MN_WriteText(tempstr, 50, 110);

  y = 103 << hires;

  // draw my computers framerate
  for(x=0; x<(scrwidth*3)/4; x++)
    {
      *(screens[0] + y*scrwidth + x) = BARCOLOR;
      if(hires)
	*(screens[0] + (y+1)*scrwidth + x) = BARCOLOR;
    }
}

void MN_ShowFrameRate(int framerate)
{
  this_framerate = framerate;
  MN_StartMenu(&menu_framerate);
  D_StartTitle();      // user does not need to see the console
}

void MN_AddMenus()
{
  C_AddCommand(mn_newgame);
  C_AddCommand(mn_episode);
  C_AddCommand(mn_startlevel);
  C_AddCommand(use_startmap);
  
  C_AddCommand(mn_features);
  C_AddCommand(mn_loadwad);
  C_AddCommand(mn_wadname);
  C_AddCommand(mn_demos);
  C_AddCommand(mn_demoname);
  
  C_AddCommand(mn_multi);
  C_AddCommand(mn_serial);
  C_AddCommand(mn_phonenum);
  C_AddCommand(mn_tcpip);
  C_AddCommand(mn_chatmacros);
  C_AddCommand(mn_player);
  C_AddCommand(mn_advanced);
  
  // different connect types
  C_AddCommand(mn_ser_answer);
  C_AddCommand(mn_ser_connect);
  C_AddCommand(mn_udpserv);
  C_AddCommand(mn_startgame);
  C_AddCommand(mn_multigame);
  
  C_AddCommand(mn_options);
  C_AddCommand(mn_mouse);
  C_AddCommand(mn_video);
  C_AddCommand(mn_sound);
  C_AddCommand(mn_weapons);
  C_AddCommand(mn_compat);
  C_AddCommand(mn_enemies);
  C_AddCommand(mn_hud);
  C_AddCommand(mn_status);
  C_AddCommand(mn_automap);

  C_AddCommand(newgame);

  // prompt messages
  C_AddCommand(mn_quit);
  C_AddCommand(mn_endgame);
}

