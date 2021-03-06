// Emacs style mode select -*- C++ -*-
//--------------------------------------------------------------------------
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
// Demo Handling
//
// Moved out of the g_ code and into the netcode
//
//--------------------------------------------------------------------------

#include "c_io.h"
#include "c_runcmd.h"
#include "doomdef.h"
#include "doomstat.h"
#include "cl_clien.h"
#include "d_main.h"
#include "g_game.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_random.h"
#include "sv_serv.h"
#include "w_wad.h"
#include "z_zone.h"

#define MIN_MAXPLAYERS 32

#define DEMOMARKER    0x80

extern int nettics[MAXPLAYERS]; // cl_clien.c

extern boolean netdemo;

static byte *demobuffer;
static byte *demo_p;           // read/write pt in buffer

boolean demoplayback;
boolean demorecording;

// for comparative timing purposes

static int startgametic;
static int starttime;
static boolean timedemo_menuscreen;

//===========================================================================
//
// General Demo Functions
//
//===========================================================================

extern boolean advancedemo;

//---------------------------------------------------------------------------
//
// G_CheckDemoStatus
//
// Called when we reach the end of a demo file
//

boolean G_CheckDemoStatus() {}

//
// CL_EndDemo
//
// This is called when we reach the end of the currently
// playing demo. If advance is true, we call D_AdvanceDemo
// to advance to the next demo in the list.
//

static void CL_EndDemo(boolean advance)
{
  if(demorecording)
    {
    }
  else if(demoplayback)
    {
      if(timingdemo)
	{
	  int gametics = gametic - startgametic;
	  int realtics = I_GetTime_RealTime() - starttime;
	  
	  timingdemo = singletics = false;

	  if(realtics <= 0)     // catch div-by-zero?
	    realtics = 1;

	  if(timedemo_menuscreen)
	    MN_ShowFrameRate((gametics * TICRATE * 10) / realtics);
	  else
	    C_Printf("%i fps\n", (gametics * TICRATE) / realtics);

	  C_Printf("%i %i %i\n", starttime, I_GetTime_RealTime(),
		   gametics);
	}

      Z_ChangeTag(demobuffer, PU_CACHE);
      G_ReloadDefaults();    // killough 3/1/98
      netgame = netdemo = false;       // killough 3/29/98
      deathmatch = false;
      demoplayback = false;
      if (singledemo || !advance)
	{
	  demoplayback = false;
	  C_SetConsole();
	  return false;
	}

      // next demo      

      D_AdvanceDemo();

      return true;
    }

  return false;
}

void CL_StopDemo()
{
  CL_EndDemo(false);
}

void G_StopDemo()
{
  CL_StopDemo();
}

//===========================================================================
//
// Demo Playback
//
//===========================================================================

// read ticcmd from original doom demo

static void CL_ReadOrigCmd(ticcmd_t *cmd)
{
  cmd->forwardmove = ((signed char)*demo_p++);
  cmd->sidemove = ((signed char)*demo_p++);
  cmd->angleturn = ((unsigned char)*demo_p++)<<8;
  cmd->buttons = (unsigned char)*demo_p++;
  cmd->updownangle=0;

  // killough 3/26/98, 10/98: Ignore savegames in demos 
  if (demoplayback && 
      cmd->buttons & BT_SPECIAL &&
      cmd->buttons & BTS_SAVEGAME)
    {
      cmd->buttons &= ~BTS_SAVEGAME;
      doom_printf("Game Saved (Suppressed)");
    }
}

//--------------------------------------------------------------------------
//
// CL_ReadDemoCmd
//
// Read next ticcmd from demo
//

void CL_ReadDemoCmd(ticcmd_t *ticcmd)
{
  if(*demo_p == DEMOMARKER)
    {
      // end of demo
      CL_EndDemo(true);
    }
  else
    {
      // read next ticcmd from file

      CL_ReadOrigCmd(ticcmd);
    }
}


//-------------------------------------------------------------------------
//
// CL_PlayDemo
//
// Load demo and start playing
//

void CL_PlayDemo(char *demoname)
{
  int length;
  int demover;
  byte *option_p = NULL;      // killough 11/98
  skill_t skill;
  int i, episode, map;

  // stop any running demos first
  
  CL_StopDemo();
  
  // sf: try reading from a file first
  // we no longer have to load the demo lmp into the wad directory
  
  length = M_ReadFile(demoname, &demobuffer);

  // file not found ?
  
  if(length <= 0)
    {
      int lumpnum;
      char basename[9];

      // try a lump instead
      ExtractFileBase(demoname, basename);           // killough

      // check if lump exists
      if((lumpnum = W_CheckNumForName(basename)) == -1)
	{
	  C_Printf("not found: %s\n", demoname);
	  return;
	}
      else
	{
	  demobuffer = W_CacheLumpNum (lumpnum, PU_STATIC);  // killough
	}      
    }
      
  demo_p = demobuffer;

  //-------------------------------------------------------------------------
  //
  // Read Demo Header
  //
  
  
  // killough 2/22/98, 2/28/98: autodetect old demos and act accordingly.
  // Old demos turn on demo_compatibility => compatibility; new demos load
  // compatibility flag, and other flags as well, as a part of the demo.

  demo_version =      // killough 7/19/98: use the version id stored in demo
    demover = *demo_p++;

  if (demover < 200)     // Autodetect old demos
    {
      compatibility = true;
      memset(comp, 0xff, sizeof comp);  // killough 10/98: a vector now

      // killough 3/2/98: force these variables to be 0 in demo_compatibility

      variable_friction = 0;

      weapon_recoil = 0;

      allow_pushers = 0;

      monster_infighting = 1;           // killough 7/19/98

      bfgtype = bfg_normal;                  // killough 7/19/98

#ifdef DOGS
      dogs = 0;                         // killough 7/19/98
      dog_jumping = 0;                  // killough 10/98
#endif

      monster_backing = 0;              // killough 9/8/98
      
      monster_avoid_hazards = 0;        // killough 9/9/98

      monster_friction = 0;             // killough 10/98
      help_friends = 0;                 // killough 9/9/98
      monkeys = 0;

      // killough 3/6/98: rearrange to fix savegame bugs (moved fastparm,
      // respawnparm, nomonsters flags to G_LoadOptions()/G_SaveOptions())

      if ((skill=demover) >= 100)         // For demos from versions >= 1.4
	{
	  skill = *demo_p++;
	  episode = *demo_p++;
	  map = *demo_p++;
	  deathmatch = *demo_p++;
	  respawnparm = *demo_p++;
	  fastparm = *demo_p++;
	  nomonsters = *demo_p++;
	  consoleplayer = *demo_p++;
	}
      else
	{
	  episode = *demo_p++;
	  map = *demo_p++;
	  deathmatch = respawnparm = fastparm =
	    nomonsters = consoleplayer = 0;
	}
    }
  else    // new versions of demos (boom)
    {
      demo_p += 6;               // skip signature;

      compatibility = *demo_p++;       // load old compatibility flag
      skill = *demo_p++;
      episode = *demo_p++;
      map = *demo_p++;
      deathmatch = *demo_p++;
      consoleplayer = *demo_p++;

      // killough 11/98: save option pointer for below
      if (demover >= 203)
	option_p = demo_p;

      demo_p = G_ReadOptions(demo_p);  // killough 3/1/98: Read game options

      if (demover == 200)        // killough 6/3/98: partially fix v2.00 demos
	demo_p += 256-GAME_OPTION_SIZE;
    }

  if (demo_compatibility)  // only 4 players can exist in old demos
    {
      for (i=0; i<4; i++)  // intentionally hard-coded 4 -- killough
	playeringame[i] = *demo_p++;
      for (;i < MAXPLAYERS; i++)
	playeringame[i] = 0;
    }
  else
    {
      for (i=0 ; i < MAXPLAYERS; i++)
	playeringame[i] = *demo_p++;
      demo_p += MIN_MAXPLAYERS - MAXPLAYERS;
    }
  
  if (playeringame[1])
    netgame = netdemo = true;

  // don't spend a lot of time in loadlevel

  if (gameaction != ga_loadgame)      // killough 12/98: support -loadgame
    {
      // killough 2/22/98:
      // Do it anyway for timing demos, to reduce timing noise
      precache = timingdemo;
  
      G_InitNewNum(skill, episode, map);

      // killough 11/98: If OPTIONS were loaded from the wad in G_InitNew(),
      // reload any demo sync-critical ones from the demo itself, to be exactly
      // the same as during recording.
      
      if (option_p)
	G_ReadOptions(option_p);
    }

  precache = true;
  usergame = false;
  demoplayback = true;
  singledemo = false;

  for(i=0; i<MAXPLAYERS; i++)
    nettics[i] = gametic;
  basetic = gametic;
  
  for (i=0; i<MAXPLAYERS;i++)         // killough 4/24/98
    players[i].cheats = 0;
}

void CL_TimeDemo(char *name)
{
  starttime = I_GetTime_RealTime();
  startgametic = gametic;

  CL_PlayDemo(name);

  timingdemo = true;
  singledemo = true;
  singletics = true;
  timedemo_menuscreen = cmdtype == c_menu;
}

CONSOLE_COMMAND(playdemo, cf_notnet)
{
  CL_PlayDemo(c_argv[0]);
  singledemo = true;            // quit after one demo
}

CONSOLE_COMMAND(timedemo, cf_notnet)
{
  CL_TimeDemo(c_argv[0]);
}

void G_RecordDemo(char *name) {}



//==========================================================================
//
// Console Commands
//
//==========================================================================

void CL_Demo_AddCommands()
{
  C_AddCommand(playdemo);
  C_AddCommand(timedemo);
}


/************************ OLD CODE ***********************************/

#if 0

//==========================================================================
//
// Demo Playback
//
// Load new demo file, and functions to return the next ticcmd from the
// demo buffer
//
//==========================================================================

static byte     *demo_p;
static char     *demobuffer;
static size_t   maxdemosize;

boolean         timedemo_menuscreen;
boolean         demorecording;
boolean         demoplayback;

//-------------------------------------------------------------------------
//
// CL_CloseDemo
//
// Close demo file
//

boolean CL_CloseDemo(void)
{
#if 0
  if (demorecording)
    {
      demorecording = false;
      *demo_p++ = DEMOMARKER;

      if (!M_WriteFile(demoname, demobuffer, demo_p - demobuffer))
	I_Error("Error recording demo %s: %s", demoname,  // killough 11/98
		errno ? strerror(errno) : "(Unknown Error)");

      free(demobuffer);
      demobuffer = NULL;  // killough
      I_Error("Demo %s recorded",demoname);
      return false;  // killough
    }
#endif
  
  if (timingdemo)
    {
      int endtime = I_GetTime_RealTime();
      // killough -- added fps information and made it work for longer demos:
      unsigned realtics = endtime-starttime;
      unsigned gametics = gametic-startgametic;
      C_Printf ("\n" FC_GRAY "%-.1f frames per second\n",
               (unsigned) gametics * (double) TICRATE / realtics);
      singletics = false;
      demoplayback = false;
      Z_ChangeTag(demobuffer, PU_CACHE);
      G_ReloadDefaults();    // killough 3/1/98
      netgame = false;       // killough 3/29/98
      deathmatch = false;
      timingdemo = false;
      C_SetConsole();
      ResetNet();
      // check for timedemo from menu
      if(timedemo_menuscreen)
	MN_ShowFrameRate((gametics * TICRATE * 10) / realtics);
      return false;
    }              

  if (demoplayback)
    {
      Z_ChangeTag(demobuffer, PU_CACHE);
      G_ReloadDefaults();    // killough 3/1/98
      netgame = false;       // killough 3/29/98
      deathmatch = false;
      if (singledemo)
	{
          demoplayback = false;
          C_SetConsole();
          return false;
	}
      D_AdvanceDemo();
      return true;
    }
  
  return false;
}

//--------------------------------------------------------------------------
//
// CL_ReadDemoTiccmd
//
// Read next ticcmd from demo file
//

void CL_ReadDemoTiccmd(ticcmd_t *cmd)
{
  if (*demo_p == DEMOMARKER)
    G_CheckDemoStatus();      // end of demo data stream
  else
    {
      cmd->forwardmove = ((signed char)*demo_p++);
      cmd->sidemove = ((signed char)*demo_p++);
      cmd->angleturn = ((unsigned char)*demo_p++)<<8;
      cmd->buttons = (unsigned char)*demo_p++;
      cmd->updownangle=0;

      // killough 3/26/98, 10/98: Ignore savegames in demos 
      if (demoplayback && 
	  cmd->buttons & BT_SPECIAL &&
	  cmd->buttons & BTS_SAVEGAME)
	{
	  cmd->buttons &= ~BTS_SAVEGAME;
          doom_printf("Game Saved (Suppressed)");
	}
    }
}

//------------------------------------------------------------------------
//
// CL_PlayDemo
//
// Play demo from wad resource or a file
//

void CL_PlayDemo(char *demoname)
{
  skill_t skill;
  int i, episode, map;
  int demover;
  byte *option_p = NULL;      // killough 11/98
  int lumpnum;
  int length;

  gametic = gametime = 0;

  for(i=0; i<MAXPLAYERS; i++)
    nettics[i] = 0;
  
  if (gameaction != ga_loadgame)      // killough 12/98: support -loadgame
    basetic = gametic;  // killough 9/29/98

  gameaction = ga_nothing;

  // sf: try reading from a file first
  // we no longer have to load the demo lmp into the wad directory
  
  length = M_ReadFile(demoname, &demobuffer);

  // file not found ?
  
  if(length <= 0)
    {
      char basename[9];

      // try a lump instead
      ExtractFileBase(demoname,basename);           // killough

      // check if lump exists
      if(-1 == (lumpnum = W_CheckNumForName(basename)))
	{
	  C_Printf("not found: %s\n", demoname);
	  return;
	}
      else
	{
	  demobuffer = W_CacheLumpNum (lumpnum, PU_STATIC);  // killough
	}      
    }
      
  demo_p = demobuffer;
  
  // killough 2/22/98, 2/28/98: autodetect old demos and act accordingly.
  // Old demos turn on demo_compatibility => compatibility; new demos load
  // compatibility flag, and other flags as well, as a part of the demo.

  demo_version =      // killough 7/19/98: use the version id stored in demo
  demover = *demo_p++;

  if (demover < 200)     // Autodetect old demos
    {
      compatibility = true;
      memset(comp, 0xff, sizeof comp);  // killough 10/98: a vector now

      // killough 3/2/98: force these variables to be 0 in demo_compatibility

      variable_friction = 0;

      weapon_recoil = 0;

      allow_pushers = 0;

      monster_infighting = 1;           // killough 7/19/98

      bfgtype = bfg_normal;                  // killough 7/19/98

#ifdef DOGS
      dogs = 0;                         // killough 7/19/98
      dog_jumping = 0;                  // killough 10/98
#endif

      monster_backing = 0;              // killough 9/8/98
      
      monster_avoid_hazards = 0;        // killough 9/9/98

      monster_friction = 0;             // killough 10/98
      help_friends = 0;                 // killough 9/9/98
      monkeys = 0;

      // killough 3/6/98: rearrange to fix savegame bugs (moved fastparm,
      // respawnparm, nomonsters flags to G_LoadOptions()/G_SaveOptions())

      if ((skill=demover) >= 100)         // For demos from versions >= 1.4
	{
	  skill = *demo_p++;
	  episode = *demo_p++;
	  map = *demo_p++;
	  deathmatch = *demo_p++;
	  respawnparm = *demo_p++;
	  fastparm = *demo_p++;
	  nomonsters = *demo_p++;
	  consoleplayer = *demo_p++;
	}
      else
	{
	  episode = *demo_p++;
	  map = *demo_p++;
	  deathmatch = respawnparm = fastparm =
	    nomonsters = consoleplayer = 0;
	}
    }
  else    // new versions of demos
    {
      demo_p += 6;               // skip signature;

      compatibility = *demo_p++;       // load old compatibility flag
      skill = *demo_p++;
      episode = *demo_p++;
      map = *demo_p++;
      deathmatch = *demo_p++;
      consoleplayer = *demo_p++;

      // killough 11/98: save option pointer for below
      if (demover >= 203)
	option_p = demo_p;

      demo_p = G_ReadOptions(demo_p);  // killough 3/1/98: Read game options

      if (demover == 200)        // killough 6/3/98: partially fix v2.00 demos
	demo_p += 256-GAME_OPTION_SIZE;
    }

  if (demo_compatibility)  // only 4 players can exist in old demos
    {
      for (i=0; i<4; i++)  // intentionally hard-coded 4 -- killough
	playeringame[i] = *demo_p++;
      for (;i < MAXPLAYERS; i++)
	playeringame[i] = 0;
    }
  else
    {
      for (i=0 ; i < MAXPLAYERS; i++)
	playeringame[i] = *demo_p++;
      demo_p += MIN_MAXPLAYERS - MAXPLAYERS;
    }

  if (playeringame[1])
    netgame = netdemo = true;

  // don't spend a lot of time in loadlevel

  if (gameaction != ga_loadgame)      // killough 12/98: support -loadgame
    {
      // killough 2/22/98:
      // Do it anyway for timing demos, to reduce timing noise
      precache = timingdemo;
  
      G_InitNewNum(skill, episode, map);

      // killough 11/98: If OPTIONS were loaded from the wad in G_InitNew(),
      // reload any demo sync-critical ones from the demo itself, to be exactly
      // the same as during recording.
      
      if (option_p)
	G_ReadOptions(option_p);
    }

  precache = true;
  usergame = false;
  demoplayback = true;

  for (i=0; i<MAXPLAYERS;i++)         // killough 4/24/98
    players[i].cheats = 0;

  if (timingdemo)
    {
      starttime = I_GetTime_RealTime();
      startgametic = gametic;
    }
}

#if 0

// Demo limits removed -- killough

static void G_WriteDemoTiccmd(ticcmd_t* cmd)
{
  int position = demo_p - demobuffer;

  demo_p[0] = cmd->forwardmove;
  demo_p[1] = cmd->sidemove;
  demo_p[2] = (cmd->angleturn+128)>>8;
  demo_p[3] = cmd->buttons;

  if (position+16 > maxdemosize)   // killough 8/23/98
    {
      // no more space
      maxdemosize += 128*1024;   // add another 128K  -- killough
      demobuffer = realloc(demobuffer,maxdemosize);
      demo_p = position + demobuffer;  // back on track
      // end of main demo limit changes -- killough
    }

  G_ReadDemoTiccmd (cmd);         // make SURE it is exactly the same
}

//
// G_RecordDemo
//

void G_RecordDemo(char *name)
{
  int i;

  demo_insurance = default_demo_insurance!=0;     // killough 12/98
      
  usergame = false;
  AddDefaultExtension(strcpy(demoname, name), ".lmp");  // 1/18/98 killough
  i = M_CheckParm ("-maxdemo");
  if (i && i<myargc-1)
    maxdemosize = atoi(myargv[i+1])*1024;
  if (maxdemosize < 0x20000)  // killough
    maxdemosize = 0x20000;
  demobuffer = malloc(maxdemosize); // killough
  demorecording = true;
}

//
// G_PlayDemo
//

void G_DeferedPlayDemo(char *s)
{
  static char name[100];

  while(*s==' ') s++;             // catch invalid demo names
  /*
  if(W_CheckNumForName(s) == -1)
    {
      C_Printf("%s: demo not found\n",s);
      return;
    }
    */
  strcpy(name, s);
  
  G_StopDemo();          // stop any previous demos
  
  defdemoname = name;
  gameaction = ga_playdemo;
  singledemo = false;      // sf: moved from reloaddefaults
}

// G_TimeDemo - sf

void G_TimeDemo(char *s)
{
  static char name[100];

  while(*s==' ') s++;             // catch invalid demo names

  /*
  if(W_CheckNumForName(s) == -1)
    {
      C_Printf("%s: demo not found\n",s);
      return;
    }
    */
  
  strcpy(name, s);
  
  G_StopDemo();          // stop any previous demos
  
  defdemoname = name;
  gameaction = ga_playdemo;
  singledemo = true;      // sf: moved from reloaddefaults

  singletics = true;
  timingdemo = true;            // show stats after quit

  // check for framerate checking from menu

  if(menuactive)
    timedemo_menuscreen = true;
  else
    timedemo_menuscreen = false; // from console
}

void G_StopDemo()
{
  extern boolean advancedemo;

  DEBUGMSG("    stop demo\n");

  if(!demorecording && !demoplayback) return;
  
  G_CheckDemoStatus();
  advancedemo = false;
  C_SetConsole();
}
#endif
#endif

//--------------------------------------------------------------------------
//
// $Log$
// Revision 1.4  2001-01-18 01:35:52  fraggle
// fix up demo code somewhat
//
// Revision 1.3  2001/01/13 00:33:55  fraggle
// fix/change fps menu
//
// Revision 1.2  2000/05/10 13:11:37  fraggle
// fix demos
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//--------------------------------------------------------------------------
