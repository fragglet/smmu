// Emacs style mode select -*- C++ -*-
//--------------------------------------------------------------------------
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
// Network Menus
//
//--------------------------------------------------------------------------

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "m_argv.h"
#include "d_player.h"
#include "mn_engin.h"
#include "net_modl.h"
#include "r_draw.h"
#include "v_video.h"
#include "v_misc.h"
#include "w_wad.h"
#include "z_zone.h"

#define NOTCPIP  /* no tcp/ip error msg */                \
            "tcp/ip not detected. make sure that\n"       \
	    "you are in windows and have winsock.\n"      \
	    "winsock2 users need to run winsock2.bat"

#define NOEXTERN /* external program not loaded */        \
	    "you must run the game through a driver:\n"   \
	    "eg. ipxsetup.exe or sersetup.exe"
	    
char *mn_phonenum;           // phone number to dial

//--------------------------------------------------------------------------
//
// Multiplayer Menu
//
// Access to the new Multiplayer features of SMMU
//

menu_t menu_multiplayer =
{
  {
    {it_title,  FC_GOLD "multiplayer",  NULL,                   "M_MULTI"},
    {it_gap},
    {it_gap},
    {it_info,   FC_GOLD "connect:"},
    {it_runcmd, "join game",            "mn_join"},
    {it_runcmd, "start server",         "mn_startserver"},
    {it_gap},
    {it_runcmd, "disconnect",           "disconnect"},
    {it_gap},
    {it_info,   FC_GOLD "setup"},
    {it_runcmd, "chat macros",          "mn_chatmacros"},
    {it_runcmd, "player setup",         "mn_player"},
    {it_runcmd, "game settings",        "mn_setupserver"},
    {it_end},
  },
  100, 15,                                      // x,y offsets
  mf_background|mf_leftaligned,                 // fullscreen
};

CONSOLE_COMMAND(mn_multi, 0)
{
  MN_StartMenu(&menu_multiplayer);
}

//========================================================================
//
// Connect to Server
//
//========================================================================

menu_t menu_join =
{
  {
    {it_title,  FC_GOLD "multiplayer",  NULL,                   "M_MULTI"},
    {it_gap},
    {it_info,   FC_GOLD "choose connection type:"},
    {it_gap},
#ifdef TCPIP
    {it_runcmd, "tcp/ip (internet)",    "mn_cl_inet"},
    {it_runcmd, "tcp/ip (lan)",         "mn_cl_lan"},
#endif
#ifdef DJGPP
    {it_runcmd, "serial cable",         "mn_cl_cable"},
    {it_runcmd, "modem",                "mn_cl_modem"},
    {it_runcmd, "external program",     "mn_cl_extern"},
#endif
    {it_end},    
  },
  50, 15,                                      // x,y offsets
  mf_background|mf_leftaligned,                 // fullscreen
};

CONSOLE_COMMAND(mn_join, 0)
{
  MN_StartMenu(&menu_join);
}

#ifdef TCPIP

//--------------------------------------------------------------------------
//
// LAN TCP/IP Lookup
//

menu_t menu_cl_lan =
{
  {
    {it_title,     FC_GOLD "tcp/ip",               NULL,          "M_TCPIP"},
    {it_gap},
    {it_info,      FC_GOLD "tcp/ip options"},
    {it_variable,  "udp port",                  "udp_port"},
    {it_variable,  "location",                  "mn_location"},
    {it_gap},
    {it_runcmd,    "search for servers",        "find_servers udp"},
    {it_end},
  },
  150, 15,                                      // x,y offsets
  mf_background,                                // fullscreen
};

CONSOLE_COMMAND(mn_cl_lan, 0)
{
  if(tcpip_support)
    {      
      MN_StartMenu(&menu_cl_lan);
    }
  else
    {
      MN_ErrorMsg(NOTCPIP);
    }
}

//--------------------------------------------------------------------------
//
// Internet lookup
//

menu_t menu_cl_inet =
{
  {
    {it_title,     FC_GOLD "tcp/ip",               NULL,          "M_TCPIP"},
    {it_gap},
    {it_info,      FC_GOLD "tcp/ip options"},
    {it_variable,  "udp port",                  "udp_port"},
    {it_variable,  "lookup server",             "inet_server"},
    {it_gap},
    {it_runcmd,    "find servers",              "find_servers internet"},
    {it_end},
  },
  100, 15,                                      // x,y offsets
  mf_background,                                // fullscreen
};

CONSOLE_COMMAND(mn_cl_inet, 0)
{
  if(tcpip_support)
    {
      MN_StartMenu(&menu_cl_inet);
    }
  else
    {
      MN_ErrorMsg(NOTCPIP);
    }

}

#endif /* TCPIP */

#ifdef DJGPP 

//--------------------------------------------------------------------------
//
// Serial Cable
//

menu_t menu_cl_serial =
{
  {
    {it_title,  FC_GOLD "Serial/modem",          NULL,           "M_SERIAL"},
    {it_gap},
    {it_info,   FC_GOLD "settings"},
    {it_toggle, "com port to use",               "ser_comport"},
    {it_gap},
    {it_runcmd, "connect",                       "connect serial"},
    {it_end},
  },
  180,15,                       // x,y offset
  mf_background,                // fullscreen
};

CONSOLE_COMMAND(mn_cl_cable, 0)
{
  MN_StartMenu(&menu_cl_serial);
}

//---------------------------------------------------------------------------
//
// Modem
//

menu_t menu_cl_modem =
{
  {
    {it_title,    FC_GOLD "Serial/modem",          NULL,           "M_SERIAL"},
    {it_gap},
    {it_info,     FC_GOLD "settings"},
    {it_toggle,   "com port to use",               "ser_comport"},
    {it_variable, "phone number",                  "ser_phonenum"},
    {it_gap},
    {it_runcmd,   "connect",                       "connect modem"},
    {it_end},
  },
  180,15,                       // x,y offset
  mf_background,                // fullscreen
};

CONSOLE_COMMAND(mn_cl_modem, 0)
{
  MN_StartMenu(&menu_cl_modem);
}

//--------------------------------------------------------------------------
//
// External Driver
//

CONSOLE_COMMAND(mn_cl_extern, 0)
{
  if(M_CheckParm("-net"))
    {
      cmdtype = c_menu;
      C_RunTextCmd("find_servers ext");
    }
  else
    MN_ErrorMsg(NOEXTERN);
}

#endif /* DJGPP */

//==========================================================================
//
// Start Server
//
//==========================================================================

char *start_cmd;       // command to start server

//--------------------------------------------------------------------------
//
// Start Server Menu
//

menu_t menu_startserver =
{
  {
    {it_title,  FC_GOLD "multiplayer",  NULL,                   "M_MULTI"},
    {it_gap},
    {it_info,   FC_GOLD "choose connection type:"},
    {it_gap},
#ifdef TCPIP
    {it_runcmd, "tcp/ip (internet)",    "mn_sv_inet"},
    {it_runcmd, "tcp/ip (lan)",         "mn_sv_lan"},
#endif
#ifdef DJGPP
    {it_runcmd, "serial cable",         "mn_sv_cable"},
    {it_runcmd, "modem",                "mn_sv_modem"},
    {it_runcmd, "external program",     "mn_sv_extern"},
#endif
    {it_end},    
  },
  50, 15,                                      // x,y offsets
  mf_background|mf_leftaligned,                 // fullscreen
};

CONSOLE_COMMAND(mn_startserver, 0)
{
  MN_StartMenu(&menu_startserver);
}

#ifdef TCPIP

//-------------------------------------------------------------------------
//
// Internet TCP/IP
//

menu_t menu_sv_tcpip =
{
  {
    {it_title,     FC_GOLD "tcp/ip",               NULL,          "M_TCPIP"},
    {it_gap},
    {it_info,      FC_GOLD "tcp/ip options"},
    {it_variable,  "udp port",                  "udp_port"},
    {it_variable,  "lookup server",             "inet_server"},
    {it_gap},
    {it_runcmd,    "start server",              "mn_multigame"},
    {it_end},
  },
  100, 15,                                      // x,y offsets
  mf_background,                                // fullscreen
};

CONSOLE_COMMAND(mn_sv_inet, 0)
{
  if(tcpip_support)
    {
      start_cmd = "inet_add; server udp";
      MN_StartMenu(&menu_sv_tcpip);
    }
  else
    {
      MN_ErrorMsg(NOTCPIP);
    }
}

CONSOLE_COMMAND(mn_sv_lan, 0)
{
  if(tcpip_support)
    {
      start_cmd = "server udp";
      MN_StartMenu(&menu_sv_tcpip);
    }
  else
    {
      MN_ErrorMsg(NOTCPIP);
    }

}

#endif

#ifdef DJGPP

//--------------------------------------------------------------------------
//
// Modem
//

menu_t menu_sv_serial =
{
  {
    {it_title,  FC_GOLD "Serial/modem",          NULL,           "M_SERIAL"},
    {it_gap},
    {it_info,   FC_GOLD "settings"},
    {it_toggle, "com port to use",               "ser_comport"},
    {it_gap},
    {it_runcmd, "start server",                  "mn_multigame"},
    {it_end},
  },
  180,15,                       // x,y offset
  mf_background,                // fullscreen
};

VARIABLE_STRING(mn_phonenum,     NULL,           126);
CONSOLE_VARIABLE(mn_phonenum,    mn_phonenum,     0) {}

CONSOLE_COMMAND(mn_sv_cable, 0)
{
  start_cmd = "server serial";
  MN_StartMenu(&menu_sv_serial);
}

CONSOLE_COMMAND(mn_sv_modem, 0)
{
  start_cmd = "server modem";
  MN_StartMenu(&menu_sv_serial);
}

//--------------------------------------------------------------------------
//
// External Driver
//

CONSOLE_COMMAND(mn_sv_extern, 0)
{
  if(!M_CheckParm("-net"))
    {
      MN_ErrorMsg(NOEXTERN);
      return;
    }

  start_cmd = "server ext";

  cmdtype = c_menu;
  C_RunTextCmd("mn_multigame");
}

#endif

//--------------------------------------------------------------------------
//
// Multiplayer Game settings
//

menu_t menu_multigame =
{
  {
    {it_title,    FC_GOLD "multiplayer",        NULL,             "M_MULTI"},
    {it_gap},
    {it_runcmd,   "done",                       "mn_startgame"},
    {it_gap},
    {it_info,     FC_GOLD "game settings"},
    {it_toggle,   "game type",                  "deathmatch"},
    {it_variable, "starting level",             "startlevel"},
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
  mf_background,                // full screen
};

        // level to start on
CONSOLE_STRING(startlevel, startlevel, NULL, 8, cf_handlerset|cf_nosave)
{
  char *newvalue = c_argv[0];

  // check for a valid level
  if(W_CheckNumForName(newvalue) == -1)
    MN_ErrorMsg("level not found!");
  else
    {
      if(startlevel) Z_Free(startlevel);
      startlevel = Z_Strdup(newvalue, PU_STATIC, 0);
    }
}

CONSOLE_COMMAND(mn_multigame, 0)            // just setting options from menu
{
  MN_StartMenu(&menu_multigame);
}

CONSOLE_COMMAND(mn_setupserver, 0)
{
  start_cmd = "mn_prevmenu";
  MN_StartMenu(&menu_multigame);
}

// start game

CONSOLE_COMMAND(mn_startgame, 0)
{
  cmdtype = c_menu;
  C_RunTextCmd(start_cmd);
}

//--------------------------------------------------------------------------
//
// Multiplayer Game settings
// Advanced menu
//

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
  mf_background,                // full screen
};

CONSOLE_COMMAND(mn_advanced, cf_server)
{
  MN_StartMenu(&menu_advanced);
}

//--------------------------------------------------------------------------
//
// Chat Macros
//

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
  mf_background,                        // full-screen
};

CONSOLE_COMMAND(mn_chatmacros, 0)
{
  MN_StartMenu(&menu_chatmacros);
}

//--------------------------------------------------------------------------
//
// Player Setup
//

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
    {it_toggle,         "handedness",           "lefthanded"},
    {it_end}
  },
  150,5,                                // x, y offset
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

//-------------------------------------------------------------------------
//
// Add Console Commands
//

void MN_Net_AddCommands()
{
  C_AddCommand(mn_multi);
      C_AddCommand(mn_join);
      C_AddCommand(mn_startserver);
	  C_AddCommand(mn_multigame);
	  C_AddCommand(mn_setupserver);
	  C_AddCommand(startlevel);
	  C_AddCommand(mn_startgame);
      
      C_AddCommand(mn_chatmacros);

      C_AddCommand(mn_player);

#ifdef TCPIP
  C_AddCommand(mn_cl_lan);
  C_AddCommand(mn_cl_inet);
  C_AddCommand(mn_sv_lan);
  C_AddCommand(mn_sv_inet);
#endif
  
#ifdef DJGPP
  C_AddCommand(mn_cl_extern);
  C_AddCommand(mn_cl_modem);
  C_AddCommand(mn_cl_cable);
  C_AddCommand(mn_sv_extern);
  C_AddCommand(mn_sv_modem);
  C_AddCommand(mn_sv_cable);
#endif

}

//-------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:09  fraggle
// Initial revision
//
//
//-------------------------------------------------------------------------
