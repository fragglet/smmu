// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// DESCRIPTION:  none
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id$";

#include <time.h>
#include <stdarg.h>
#include <errno.h>

#include "c_io.h"
#include "c_net.h"
#include "c_runcmd.h"
#include "p_info.h"
#include "doomstat.h"
#include "f_finale.h"
#include "f_wipe.h"
#include "m_argv.h"
#include "m_misc.h"
#include "mn_engin.h"
#include "mn_menus.h"
#include "m_random.h"
#include "p_setup.h"
#include "p_saveg.h"
#include "p_tick.h"
#include "d_main.h"
#include "wi_stuff.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "am_map.h"
#include "w_wad.h"
#include "r_main.h"
#include "r_draw.h"
#include "p_map.h"
#include "s_sound.h"
#include "dstrings.h"
#include "sounds.h"
#include "p_chase.h"
#include "r_data.h"
#include "r_sky.h"
#include "d_deh.h"              // Ty 3/27/98 deh declarations
#include "p_inter.h"
#include "g_game.h"
#include "g_bind.h"
#include "c_net.h"
#include "cl_clien.h"
#include "p_hubs.h"

#define SAVEGAMESIZE  0x20000
#define SAVESTRINGSIZE  24

#define MIN_MAXPLAYERS 32

extern boolean  deh_pars;
extern ticcmd_t player_cmds[MAXPLAYERS];

extern int      autorun;      // always running?          // phares
static size_t   savegamesize = SAVEGAMESIZE; // killough
boolean  netdemo;
static byte     *demobuffer;   // made some static -- killough
static byte     *demo_p;

gameaction_t    gameaction;
gamestate_t     gamestate = GS_INIT;    // use gs_init while starting doom
skill_t         gameskill;
boolean         respawnmonsters;
int             gameepisode;
int             gamemap;
char            *gamemapname;
boolean         paused;
boolean         sendpause;     // send a pause event next tic
boolean         sendsave;      // send a save event next tic
boolean         usergame;      // ok to save / end game
boolean         timingdemo;    // if true, exit with report on completion
boolean         fastdemo;      // if true, run at full speed -- killough
boolean         nodrawers;     // for comparative timing purposes
boolean         noblit;        // for comparative timing purposes
boolean         deathmatch;    // only if started as net death
boolean         netgame;       // only true if packets are broadcast
boolean         playeringame[MAXPLAYERS];
player_t        players[MAXPLAYERS];
int             consoleplayer; // player taking events and displaying
int             displayplayer; // view being displayed
int             gametic;
int             levelstarttic; // gametic at level start
int             basetic;       // killough 9/29/98: for demo sync
int             totalkills, totalitems, totalsecret;    // for intermission
boolean         singledemo;           // quit after playing a demo from cmdline
boolean         precache = true;      // if true, load all graphics at start
wbstartstruct_t wminfo;               // parms for world map / intermission
boolean         haswolflevels = false;// jff 4/18/98 wolf levels present
byte            *savebuffer;
int             automlook = false;
int             bfglook = 1;
int             smooth_turning = 0;       // sf

// sf: moved sensitivity here
int             mouseSensitivity_horiz = 8;    //  killough
int             mouseSensitivity_vert = 8;
int             invert_mouse = true;
int             animscreenshot = 0;       // animated screenshots

//
// controls (have defaults)
//

// int     key_mlook;
// int     key_lookup;
// int     key_lookdown;
// int     key_centerview;
// int     key_right;
// int     key_left;
// int     key_up;
// int     key_down;
// int     key_strafeleft;
// int     key_straferight;
// int     key_fire;
// int     key_use;
// int     key_strafe;
// int     key_speed;
int     key_escape = KEYD_ESCAPE;                           // phares 4/13/98
// int     key_savegame;                                    // phares
// int     key_loadgame;                                    //    |
int     key_autorun;                                        //    V
int     key_reverse;
int     key_zoomin;
int     key_zoomout;
int     key_chat;
int     key_backspace;
int     key_enter;
// int     key_help = KEYD_F1;                             // phares 4/13/98
// int     key_soundvolume;
// int     key_frags;
// int     key_hud;
// int     key_quicksave;
// int     key_endgame;
// int     key_messages;
// int     key_quickload;
// int     key_quit;
// int     key_gamma;
// int     key_spy;
// int     key_pause;
int     destination_keys[MAXPLAYERS];
int     key_weapontoggle;
// int     key_weapon1;
// int     key_weapon2;
// int     key_weapon3;
// int     key_weapon4;
// int     key_weapon5;
// int     key_weapon6;
// int     key_weapon7;                                        //    ^
// int     key_weapon8;                                        //    |
// int     key_weapon9;                                        // phares

int     key_screenshot;             // killough 2/22/98: screenshot key
int     key_setup;                  // killough 10/98: shortcut to setup menu

// sf: moved key_map variables to am_map.c

int     mousebfire;
int     mousebstrafe;
int     mousebforward;
int     joybfire;
int     joybstrafe;
int     joybuse;
int     joybspeed;

#define MAXPLMOVE   (forwardmove[1])
#define TURBOTHRESHOLD  0x32
#define SLOWTURNTICS  6
#define QUICKREVERSE 32768 // 180 degree reverse                    // phares

fixed_t forwardmove[2] = {0x19, 0x32};
fixed_t sidemove[2]    = {0x18, 0x28};
fixed_t angleturn[3]   = {640, 1280, 320};  // + slow turn

//boolean gamekeydown[NUMKEYS];
int     turnheld;       // for accelerative turning

boolean mousearray[4];
boolean *mousebuttons = &mousearray[1];    // allow [-1]

// mouse values are used once
int   mousex;
int   mousey;
int   dclicktime;
int   dclickstate;
int   dclicks;
int   dclicktime2;
int   dclickstate2;
int   dclicks2;

// joystick values are repeated
int   joyxmove;
int   joyymove;
boolean joyarray[5];
boolean *joybuttons = &joyarray[1];    // allow [-1]

int   savegameslot;
char  savedescription[32];

//jff 3/24/98 declare startskill external, define defaultskill here
extern skill_t startskill;      //note 0-based
int defaultskill;               //note 1-based

// killough 2/8/98: make corpse queue variable in size
int    bodyqueslot, bodyquesize, default_bodyquesize; // killough 2/8/98, 10/98

void   *statcopy;       // for statistics driver

int keylookspeed = 5;

int cooldemo = false;
int cooldemo_tics;      // number of tics until changing view

static char demoname[9];

void G_CoolViewPoint();


//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//

// sf: changed to action_ binding variables

void G_BuildTiccmd(ticcmd_t* cmd)
{
  boolean strafe;
  boolean bstrafe;
  int speed;
  int tspeed;
  int forward;
  int side;
  int newweapon;                                          // phares
  int updownangle = 0;
  int mlook = 0;
  int i;
  static int prevmlook = 0;
  ticcmd_t *base;
  int tmousex, tmousey; // local mousex, mousey

  base = I_BaseTiccmd();   // empty, or external driver
  memcpy(cmd, base, sizeof *cmd);

  strafe = action_strafe;
  speed = autorun || action_speed;
  
  forward = side = 0;

    // use two stage accelerative turning
    // on the keyboard and joystick
  if (joyxmove < 0 || joyxmove > 0 ||
      action_left || action_right)
    turnheld += ticdup;
  else
    turnheld = 0;

  if (turnheld < SLOWTURNTICS)
    tspeed = 2;             // slow turn
  else
    tspeed = speed;

  // turn 180 degrees in one keystroke?                  

  if (action_flip)
    {
      cmd->angleturn += QUICKREVERSE;
      action_flip = false;
    }

  // let movement keys cancel each other out

  if (strafe)
    {
      if(action_left)
	side -= sidemove[speed];
      if(action_right)
	side += sidemove[speed];
      if (joyxmove > 0)
	side += sidemove[speed];
      if (joyxmove < 0)
	side -= sidemove[speed];
    }
  else
    {
      if(action_left)
	cmd->angleturn += angleturn[tspeed];
      if(action_right)
	cmd->angleturn -= angleturn[tspeed];
      if (joyxmove > 0)
	cmd->angleturn -= angleturn[tspeed];
      if (joyxmove < 0)
	cmd->angleturn += angleturn[tspeed];
    }

  if (joyymove < 0)
    forward += forwardmove[speed];
  if (joyymove > 0)
    forward -= forwardmove[speed];

  if(action_forward)
    forward += forwardmove[speed];
  if(action_backward)
    forward -= forwardmove[speed];

  if(action_moveleft)
    side -= sidemove[speed];
  if(action_moveright)
    side += sidemove[speed];
  
  mlook = allowmlook && (action_mlook || automlook);

  // console commands
  for(i=0; i<CONS_BYTES; i++)
    cmd->consdata[i] = C_dequeueChatChar();

  if (action_attack)
    cmd->buttons |= BT_ATTACK;

  if(action_use)
    {
      cmd->buttons |= BT_USE;
      // clear double clicks if hit use button
      dclicks = 0;
    }

  // Toggle between the top 2 favorite weapons.                   // phares
  // If not currently aiming one of these, switch to              // phares
  // the favorite. Only switch if you possess the weapon.         // phares

  // killough 3/22/98:
  //
  // Perform automatic weapons switch here rather than in p_pspr.c,
  // except in demo_compatibility mode.
  //
  // killough 3/26/98, 4/2/98: fix autoswitch when no weapons are left
 
  if ((!demo_compatibility && players[consoleplayer].attackdown &&
       !P_CheckAmmo(&players[consoleplayer]))
      || action_nextweapon)
    newweapon = P_SwitchWeapon(&players[consoleplayer]);           // phares
  else
    {                                 // phares 02/26/98: Added gamemode checks

      // sf: changed to action_ variables
      newweapon =
	action_weapon1 ? wp_fist :    // killough 5/2/98: reformatted
	action_weapon2 ? wp_pistol :
	action_weapon3 ? wp_shotgun :
	action_weapon4 ? wp_chaingun :
	action_weapon5 ? wp_missile :
	action_weapon6 && gamemode != shareware ? wp_plasma :
	action_weapon7 && gamemode != shareware ? wp_bfg :
	action_weapon8 ? wp_chainsaw :
	action_weapon9 && gamemode == commercial ? wp_supershotgun :
	wp_nochange;

      // killough 3/22/98: For network and demo consistency with the
      // new weapons preferences, we must do the weapons switches here
      // instead of in p_user.c. But for old demos we must do it in
      // p_user.c according to the old rules. Therefore demo_compatibility
      // determines where the weapons switch is made.

      // killough 2/8/98:
      // Allow user to switch to fist even if they have chainsaw.
      // Switch to fist or chainsaw based on preferences.
      // Switch to shotgun or SSG based on preferences.
      //
      // killough 10/98: make SG/SSG and Fist/Chainsaw
      // weapon toggles optional
      
      if (!demo_compatibility && doom_weapon_toggles)
	{
	  const player_t *player = &players[consoleplayer];

	  // only select chainsaw from '1' if it's owned, it's
	  // not already in use, and the player prefers it or
	  // the fist is already in use, or the player does not
	  // have the berserker strength.

	  if (newweapon==wp_fist && player->weaponowned[wp_chainsaw] &&
	      player->readyweapon!=wp_chainsaw &&
	      (player->readyweapon==wp_fist ||
	       !player->powers[pw_strength] ||
	       P_WeaponPreferred(wp_chainsaw, wp_fist)))
	    newweapon = wp_chainsaw;

	  // Select SSG from '3' only if it's owned and the player
	  // does not have a shotgun, or if the shotgun is already
	  // in use, or if the SSG is not already in use and the
	  // player prefers it.

	  if (newweapon == wp_shotgun && gamemode == commercial &&
	      player->weaponowned[wp_supershotgun] &&
	      (!player->weaponowned[wp_shotgun] ||
	       player->readyweapon == wp_shotgun ||
	       (player->readyweapon != wp_supershotgun &&
		P_WeaponPreferred(wp_supershotgun, wp_shotgun))))
	    newweapon = wp_supershotgun;
	}
      // killough 2/8/98, 3/22/98 -- end of weapon selection changes
    }

  if (newweapon != wp_nochange)
    {
      cmd->buttons |= BT_CHANGE;
      cmd->buttons |= newweapon<<BT_WEAPONSHIFT;
    }

  //-------------------------------------------------------------------------
  // Old mouse stuff kept for convenience
  
  // forward double click
  if (mousebuttons[mousebforward] != dclickstate && dclicktime > 1 )
    {
      dclickstate = mousebuttons[mousebforward];
      if (dclickstate)
	dclicks++;
      if (dclicks == 2)
	{
	  cmd->buttons |= BT_USE;
	  dclicks = 0;
	}
      else
	dclicktime = 0;
    }
  else
    if ((dclicktime += ticdup) > 20)
      {
	dclicks = 0;
	dclickstate = 0;
      }

  // strafe double click

  bstrafe = mousebuttons[mousebstrafe] || joybuttons[joybstrafe];
  if (bstrafe != dclickstate2 && dclicktime2 > 1 )
    {
      dclickstate2 = bstrafe;
      if (dclickstate2)
	dclicks2++;
      if (dclicks2 == 2)
	{
	  cmd->buttons |= BT_USE;
	  dclicks2 = 0;
	}
      else
	dclicktime2 = 0;
    }
  else
    if ((dclicktime2 += ticdup) > 20)
      {
	dclicks2 = 0;
	dclickstate2 = 0;
      }

  //------------------------------------------------------------------------
  
  // sf: smooth out the mouse movement
  // change to use tmousex, y

        // divide by the number of new tics so each gets an equal share
  tmousex = mousex;
  tmousey = mousey;

  // we average the mouse movement as well
  // this is most important in smoothing movement

  if(smooth_turning)
    {
      static int oldmousex=0, mousex2;
      static int oldmousey=0, mousey2;
      mousex2 = tmousex; mousey2 = tmousey;
      tmousex = (tmousex + oldmousex)/2;        // average
      oldmousex = mousex2;
      tmousey = (tmousey + oldmousey)/2;        // average
      oldmousey = mousey2;
    }

  if(mlook && invert_mouse)
    tmousey = -tmousey;
  
  if (mlook)
    updownangle += tmousey;      // mlook
  else
    {                // just stopped mlooking?
      if(prevmlook)
	updownangle = -players[consoleplayer].updownangle;
      forward += tmousey;
    }
  
  prevmlook = mlook;

  if(action_lookdown)
    updownangle += keylookspeed;
  if(action_lookup)
    updownangle -= keylookspeed;

  if(action_center)
    updownangle = -players[consoleplayer].updownangle;

  if (strafe)
    side += tmousex*2;
  else
    cmd->angleturn -= tmousex*0x8;

  if (forward > MAXPLMOVE)
    forward = MAXPLMOVE;
  else if (forward < -MAXPLMOVE)
    forward = -MAXPLMOVE;
  if (side > MAXPLMOVE)
    side = MAXPLMOVE;
  else if (side < -MAXPLMOVE)
    side = -MAXPLMOVE;

  if(updownangle > 127) updownangle = 127;
  if(updownangle < -127) updownangle = -127;

  cmd->forwardmove += forward;
  cmd->sidemove += side;
  cmd->updownangle = updownangle;

  // special buttons
  if (sendpause)
    {
      sendpause = false;
      cmd->buttons = BT_SPECIAL | BTS_PAUSE;
    }

  // killough 10/6/98: suppress savegames in demos
  if (sendsave && !demoplayback)
    {
      sendsave = false;
      cmd->buttons = BT_SPECIAL | BTS_SAVEGAME | (savegameslot<<BTS_SAVESHIFT);
    }

  //  if(ticnum == newtics - 1)   // only if the last tic being built
  mousex = mousey = 0;  // clear them
}

//
// G_SetGameMap
//
// sf: from gamemapname, get gamemap and gameepisode
//

void G_SetGameMap()
{
  gamemap = G_GetMapForName(gamemapname);
  
  if(gamemode != commercial)
    {
      gameepisode = gamemap/10;
      gamemap = gamemap%10;
    }
  else
    gameepisode = 1;

  if (gameepisode < 1)
    gameepisode = 1;

  if (gamemode == retail)
    {
      if (gameepisode > 4)
	gameepisode = 4;
    }
  else
    if (gamemode == shareware)
      {
	if (gameepisode > 1)
	  gameepisode = 1; // only start episode 1 on shareware
      }
    else
      if (gameepisode > 3)
	gameepisode = 3;

  if (gamemap < 0)
    gamemap = 0;
  if (gamemap > 9 && gamemode != commercial)
    gamemap = 9;
}

//
// G_DoLoadLevel
//
// sf: made non-static so it can be accessed by p_hubs.c

extern gamestate_t wipegamestate;
extern gamestate_t oldgamestate;

void G_DoLoadLevel(void)
{
  int i;

  levelstarttic = gametic;        // for time calculation

  if (!demo_compatibility && demo_version < 203)   // killough 9/29/98
    basetic = gametic;

  gamestate = GS_LEVEL;

  P_SetupLevel (gamemapname, 0, gameskill);

  if(gamestate != GS_LEVEL)       // level load error
    {
      for(i=0;i<MAXPLAYERS;i++)
	players[i].playerstate = PST_LIVE;
      return;
    }

  for (i=0 ; i<MAXPLAYERS ; i++)
    {
      if (playeringame[i] && players[i].playerstate == PST_DEAD)
	players[i].playerstate = PST_REBORN;
      memset (players[i].frags,0,sizeof(players[i].frags));
    }

  if(!netgame || demoplayback) consoleplayer = 0;

  gameaction = ga_nothing;
  displayplayer = consoleplayer;    // view the guy you are playing
  P_ResetChasecam();    // sf: because displayplayer changed
  Z_CheckHeap();

  // clear cmd building stuff

  if(!hub_changelevel)
    {
      //      memset (gamekeydown, 0, sizeof(gamekeydown));
      joyxmove = joyymove = 0;
      mousex = mousey = 0;
      sendpause = sendsave = paused = false;
      memset (mousebuttons, 0, sizeof(mousebuttons));
      memset (joybuttons, 0, sizeof(joybuttons));
    }
      
  // killough: make -timedemo work on multilevel demos
  // Move to end of function to minimize noise -- killough 2/22/98:

  //jff 4/26/98 wake up the status bar in case were coming out of a DM demo
  // killough 5/13/98: in case netdemo has consoleplayer other than green
  ST_Start();

  C_InstaPopup();  // pop up the console

  if(!hub_changelevel)
    {
      // sf: no screen wipe while changing hub level
      if (wipegamestate == GS_LEVEL)
	wipegamestate = -1;             // force a wipe
    }
}

//
// G_Responder
// Get info needed to make ticcmd_ts for the players.
//

boolean G_Responder(event_t* ev)
{
  // allow spy mode changes even during the demo
  // killough 2/22/98: even during DM demo
  //
  // killough 11/98: don't autorepeat spy mode switch

#if 0
  
  if (ev->data1 == key_spy && netgame && (demoplayback || !deathmatch) &&
      gamestate == GS_LEVEL)
    {
      if (ev->type == ev_keyup)
	gamekeydown[key_spy] = false;
      if (ev->type == ev_keydown && !gamekeydown[key_spy])
	{
	  gamekeydown[key_spy] = true;
	  do                                          // spy mode
	    if (++displayplayer >= MAXPLAYERS)
	      displayplayer = 0;
	  while (!playeringame[displayplayer] && displayplayer!=consoleplayer);

	  ST_Start();    // killough 3/7/98: switch status bar views too
	  HU_Start();
	  S_UpdateSounds(players[displayplayer].mo);
          P_ResetChasecam();
	}
      return true;
    }

#endif
  
  if (gamestate == GS_SERVERWAIT)
    {
      CL_WaitResponder(ev);
      return true;
    }
  
  // killough 9/29/98: reformatted
  if (gamestate == GS_LEVEL && (HU_Responder(ev) ||  // chat ate the event
				ST_Responder(ev) ||  // status window ate it
				AM_Responder(ev)))   // automap ate it
    return true;

  // any other key pops up menu if in demos
  //
  // killough 8/2/98: enable automap in -timedemo demos
  //
  // killough 9/29/98: make any key pop up menu regardless of
  // which kind of demo, and allow other events during playback

  if (gameaction == ga_nothing && (demoplayback || gamestate == GS_DEMOSCREEN))
    {
      // killough 10/98:
      // Don't pop up menu, if paused in middle
      // of demo playback, or if automap active.
      // Don't suck up keys, which may be cheats

      if(!walkcam_active)       // sf: check for walkcam
      return                    // sf: fixed menu popup in demos
        (gamestate==GS_DEMOSCREEN || (demoplayback && !singledemo)) &&
	!(paused & 2) && !automapactive &&
	((ev->type == ev_keydown) ||
	 (ev->type == ev_mouse && ev->data1) ||
	 (ev->type == ev_joystick && ev->data1)) ?
        MN_StartControlPanel(), true : false;
    }

  if (gamestate == GS_FINALE && F_Responder(ev))
    return true;  // finale ate the event

  // sf: just what _was_ this doing in m_responder anyway?
  if (ev->type == ev_keydown && ev->data1 == key_autorun)      // Autorun
    {
      autorun = !autorun;
      return true;
    }

  if(ev->type == ev_keydown && ev->data1 == key_zoomin)
    {
      C_RunTextCmd("screensize +");
      return true;
    }
  if(ev->type == ev_keydown && ev->data1 == key_zoomout)
    {
      C_RunTextCmd("screensize -");
      return true;
    }

  switch (ev->type)
    {
    case ev_keydown:
      G_KeyResponder(ev);
      return true;
      
    case ev_keyup:
      G_KeyResponder(ev);
      return false;   // always let key up events filter down

    case ev_mouse:
      mousebuttons[0] = ev->data1 & 1;
      mousebuttons[1] = ev->data1 & 2;
      mousebuttons[2] = ev->data1 & 4;
      mousex = (ev->data2*(mouseSensitivity_horiz*4))/10;  // killough
      mousey = (ev->data3*(mouseSensitivity_vert))/30; //sf: vert made slower
      return true;    // eat events

    case ev_joystick:
      joybuttons[0] = ev->data1 & 1;
      joybuttons[1] = ev->data1 & 2;
      joybuttons[2] = ev->data1 & 4;
      joybuttons[3] = ev->data1 & 8;
      joyxmove = ev->data2;
      joyymove = ev->data3;
      return true;    // eat events

    default:
      break;
    }
  return false;
}

static boolean secretexit;

void G_ExitLevel(void)
{
  secretexit = false;
  gameaction = ga_completed;
}

// Here's for the german edition.
// IF NO WOLF3D LEVELS, NO SECRET EXIT!

void G_SecretExitLevel(void)
{
  secretexit = gamemode != commercial || haswolflevels;
  gameaction = ga_completed;
}

//
// G_PlayerFinishLevel
// Can when a player completes a level.
//

static void G_PlayerFinishLevel(int player)
{
  player_t *p = &players[player];
  memset(p->powers, 0, sizeof p->powers);
  memset(p->cards, 0, sizeof p->cards);
  p->mo->flags &= ~MF_SHADOW;   // cancel invisibility
  p->extralight = 0;      // cancel gun flashes
  p->fixedcolormap = 0;   // cancel ir gogles
  p->damagecount = 0;     // no palette changes
  p->bonuscount = 0;
}

//
// G_DoCompleted
//

static void G_DoCompleted(void)
{
  int i;

  gameaction = ga_nothing;

  for (i=0; i<MAXPLAYERS; i++)
    if (playeringame[i])
      G_PlayerFinishLevel(i);        // take away cards and stuff

  // clear hubs now
  P_ClearHubs();

  if (automapactive)
    AM_Stop();

  if (gamemode != commercial) // kilough 2/7/98
    switch(gamemap)
      {
      case 8:
	gameaction = ga_victory;
	return;
      case 9:
	for (i=0 ; i<MAXPLAYERS ; i++)
	  players[i].didsecret = true;
	break;
      }

  wminfo.didsecret = players[consoleplayer].didsecret;
  wminfo.epsd = gameepisode -1;
  wminfo.last = gamemap -1;

  // wminfo.next is 0 biased, unlike gamemap
  if (gamemode == commercial)
    {
      if (secretexit)
	switch(gamemap)
	  {
	  case 15:
	    wminfo.next = 30; break;
	  case 31:
	    wminfo.next = 31; break;
	  }
      else
	switch(gamemap)
	  {
	  case 31:
	  case 32:
	    wminfo.next = 15; break;
	  default:
	    wminfo.next = gamemap;
	  }
    }
  else
    {
      if (secretexit)
	wminfo.next = 8;  // go to secret level
      else
	if (gamemap == 9)
	  {
	    // returning from secret level
	    switch (gameepisode)
	      {
	      case 1:
		wminfo.next = 3;
		break;
	      case 2:
		wminfo.next = 5;
		break;
	      case 3:
		wminfo.next = 6;
		break;
	      case 4:
		wminfo.next = 2;
		break;
	      }
	  }
	else
	  wminfo.next = gamemap;          // go to next level
    }

  if(*info_nextlevel)
    {
      wminfo.next = G_GetMapForName(info_nextlevel);
      if(gamemode!=commercial) wminfo.next = wminfo.next % 10;
      wminfo.next--;
    }

  wminfo.maxkills = totalkills;
  wminfo.maxitems = totalitems;
  wminfo.maxsecret = totalsecret;
  wminfo.maxfrags = 0;
  
  // sf: moved partime code from wi_stuff.c,
  // added new features(level info)
  
  if ( gamemode == commercial )
    wminfo.partime = TICRATE * cpars[gamemap-1];
  else
    wminfo.partime = TICRATE * pars[gameepisode][gamemap];
  
  if(newlevel && !deh_pars) wminfo.partime = -1;
  if(info_partime != -1) wminfo.partime = TICRATE * info_partime;

  wminfo.pnum = consoleplayer;

  for (i=0 ; i<MAXPLAYERS ; i++)
    {
      wminfo.plyr[i].in = playeringame[i];
      wminfo.plyr[i].skills = players[i].killcount;
      wminfo.plyr[i].sitems = players[i].itemcount;
      wminfo.plyr[i].ssecret = players[i].secretcount;
      wminfo.plyr[i].stime = leveltime;
      memcpy (wminfo.plyr[i].frags, players[i].frags,
	      sizeof(wminfo.plyr[i].frags));
    }
  
  gamestate = GS_INTERMISSION;
  automapactive = false;

  if (statcopy)
    memcpy (statcopy, &wminfo, sizeof(wminfo));
  
  WI_Start (&wminfo);
}

static void G_DoWorldDone(void)
{
  idmusnum = -1;             //jff 3/17/98 allow new level's music to be loaded
  gamestate = GS_LEVEL;
  gamemap = wminfo.next+1;

  gamemapname = strdup(*info_nextlevel ? info_nextlevel :
                   G_GetNameForMap(gameepisode, gamemap) );
  hub_changelevel = false;
  G_DoLoadLevel();
  gameaction = ga_nothing;
  AM_clearMarks();           //jff 4/12/98 clear any marks on the automap
}


#define VERSIONSIZE   16

// killough 2/22/98: version id string format for savegames
#define VERSIONID "MBF %d"

static char savename[PATH_MAX+1];

//
// killough 5/15/98: add forced loadgames, which allow user to override checks
//

static boolean forced_loadgame = false;
static boolean command_loadgame = false;

void G_ForcedLoadGame(void)
{
  gameaction = ga_loadgame;
  forced_loadgame = true;
}

// killough 3/16/98: add slot info
// killough 5/15/98: add command-line

void G_LoadGame(char *name, int slot, boolean command)
{
  strcpy(savename, name);
  savegameslot = slot;
  gameaction = ga_loadgame;
  forced_loadgame = false;
  command_loadgame = command;
  hub_changelevel = false;
}

// killough 5/15/98:
// Consistency Error when attempting to load savegame.

static void G_LoadGameErr(const char *msg)
{
  Z_Free(savebuffer);                // Free the savegame buffer
  MN_ForcedLoadGame(msg);            // Print message asking for 'Y' to force
  if (command_loadgame)              // If this was a command-line -loadgame
    {
      D_StartTitle();                // Start the title screen
      gamestate = GS_DEMOSCREEN;     // And set the game state accordingly
    }
}

//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string
//

void G_SaveGame(int slot, char *description)
{
  savegameslot = slot;
  strcpy(savedescription, description);
  sendsave = true;
  hub_changelevel = false;
}

// Check for overrun and realloc if necessary -- Lee Killough 1/22/98
void CheckSaveGame(size_t size)
{
  size_t pos = save_p - savebuffer;
  size += 1024;  // breathing room
  if (pos+size > savegamesize)
    save_p = (savebuffer = realloc(savebuffer,
	   savegamesize += (size+1023) & ~1023)) + pos;
}

// killough 3/22/98: form savegame name in one location
// (previously code was scattered around in multiple places)

void G_SaveGameName(char *name, int slot)
{
  // Ty 05/04/98 - use savegamename variable (see d_deh.c)
  // killough 12/98: add .7 to truncate savegamename

  if (M_CheckParm("-cdrom"))
    sprintf(name, "c:/doomdata/%.7s%d.dsg", savegamename, slot);
  else
    sprintf(name, "%s/%.7s%d.dsg", basesavegame, savegamename, slot);
}

// killough 12/98:
// This function returns a signature for the current wad.
// It is used to distinguish between wads, for the purposes
// of savegame compatibility warnings, and options lookups.
//
// killough 12/98: use faster algorithm which has less IO

unsigned long long G_Signature(void)
{
  unsigned long long s = 0;
  int lump, i;
  //  char name[9];
  
  // sf: use gamemapname now, not gameepisode and gamemap
  lump = W_CheckNumForName(gamemapname);

  if (lump != -1 && (i = lump+10) < numlumps)
    do
      s = s*2+W_LumpLength(i);
    while (--i > lump);

  return s;
}

// sf: split into two functions

void G_SaveCurrentLevel(char *filename, char *description)
{
  int  length, i;
  char name2[VERSIONSIZE];

  save_p = savebuffer = malloc(savegamesize);

  CheckSaveGame(SAVESTRINGSIZE+VERSIONSIZE+sizeof(unsigned long));
  memcpy (save_p, description, SAVESTRINGSIZE);
  save_p += SAVESTRINGSIZE;
  memset (name2, 0, sizeof(name2));

  // killough 2/22/98: "proprietary" version string :-)
  sprintf (name2, VERSIONID, VERSION);

  memcpy (save_p, name2, VERSIONSIZE);
  save_p += VERSIONSIZE;

  // killough 2/14/98: save old compatibility flag:
  *save_p++ = compatibility;

  *save_p++ = gameskill;

  // sf: use string rather than episode, map
  {
    int i;
    for(i=0; i<8; i++)
      *save_p++ = levelmapname[i];
  }
  
  {  // killough 3/16/98, 12/98: store lump name checksum
    unsigned long long checksum = G_Signature();
    memcpy(save_p, &checksum, sizeof checksum);
    save_p += sizeof checksum;
  }

  // killough 3/16/98: store pwad filenames in savegame
  {
    char **w = wadfiles;
    for (*save_p = 0; *w; w++)
      {
	CheckSaveGame(strlen(*w)+2);
	strcat(strcat(save_p, *w), "\n");
      }
    save_p += strlen(save_p)+1;
  }

  CheckSaveGame(GAME_OPTION_SIZE+MIN_MAXPLAYERS+10);

  for (i=0 ; i<MAXPLAYERS ; i++)
    *save_p++ = playeringame[i];

  for (;i<MIN_MAXPLAYERS;i++)         // killough 2/28/98
    *save_p++ = 0;

  *save_p++ = idmusnum;               // jff 3/17/98 save idmus state

  save_p = G_WriteOptions(save_p);    // killough 3/1/98: save game options

  memcpy(save_p, &leveltime, sizeof save_p); //killough 11/98: save entire word
  save_p += sizeof save_p;

  // killough 11/98: save revenant tracer state
  *save_p++ = (gametic-basetic) & 255;

  // killough 3/22/98: add Z_CheckHeap after each call to ensure consistency

  P_NumberObjects();    // turn ptrs to numbers

  Z_CheckHeap();
  P_ArchivePlayers();
  Z_CheckHeap();
  P_ArchiveWorld();
  Z_CheckHeap();
  P_ArchiveThinkers();
  Z_CheckHeap();
  P_ArchiveSpecials();
  P_ArchiveRNG();    // killough 1/18/98: save RNG information
  Z_CheckHeap();
  P_ArchiveMap();    // killough 1/22/98: save automap information
  Z_CheckHeap();
  P_ArchiveScripts();   // sf: archive scripts
  Z_CheckHeap();

  P_DeNumberObjects();

  *save_p++ = 0xe6;   // consistancy marker

  length = save_p - savebuffer;

  Z_CheckHeap();

  if (!M_WriteFile(filename, savebuffer, length))
    doom_printf(errno ? strerror(errno) : "Could not save game: Error unknown");
  else
    if(!hub_changelevel) // sf: no 'game saved' message for hubs
      doom_printf(s_GGSAVED);  // Ty 03/27/98 - externalized

  free(savebuffer);  // killough
  savebuffer = save_p = NULL;
}

static void G_DoSaveGame()
{
  char name[PATH_MAX+1];

  G_SaveGameName(name, savegameslot);

  G_SaveCurrentLevel(name, savedescription);

  gameaction = ga_nothing;
  savedescription[0] = 0;
}

// demo functions

void G_DeferedPlayDemo(char *demo)
{
  strcpy(demoname, demo);
  gameaction = ga_playdemo;
}

void G_DeferedTimeDemo(char *demo)
{
  strcpy(demoname, demo);
  gameaction = ga_timedemo;
}


// sf: split into two functions for hubs

void G_LoadSavedLevel(char *filename)
{
  int  length, i;
  char vcheck[VERSIONSIZE];
  unsigned long long checksum;
  
  gameaction = ga_nothing;
  
  length = M_ReadFile(filename, &savebuffer);
  save_p = savebuffer + SAVESTRINGSIZE;

  // skip the description field

  // killough 2/22/98: "proprietary" version string :-)
  sprintf (vcheck, VERSIONID, VERSION);

  // killough 2/22/98: Friendly savegame version difference message
  if (!forced_loadgame && strncmp(save_p, vcheck, VERSIONSIZE))
    {
      G_LoadGameErr("Different Savegame Version!!!\n\nAre you sure?");
      return;
    }

  save_p += VERSIONSIZE;

  // killough 2/14/98: load compatibility mode
  compatibility = *save_p++;
  demo_version = VERSION;     // killough 7/19/98: use this version's id

  gameskill = *save_p++;
  
  // sf: use string rather than episode, map

  {
    int i;
    
    if(gamemapname) free(gamemapname);    //sf
    gamemapname = malloc(10);

    for(i=0; i<8; i++)
      gamemapname[i] = *save_p++;
    gamemapname[8] = 0;        // ending NULL
  }

  G_SetGameMap();       // get gameepisode, map

  if (!forced_loadgame)
   {  // killough 3/16/98, 12/98: check lump name checksum
     checksum = G_Signature();
     if (memcmp(&checksum, save_p, sizeof checksum))
       {
	 char *msg = malloc(strlen(save_p + sizeof checksum) + 128);
	 strcpy(msg,"Incompatible Savegame!!!\n");
	 if (save_p[sizeof checksum])
	   strcat(strcat(msg,"Wads expected:\n\n"), save_p + sizeof checksum);
	 strcat(msg, "\nAre you sure?");
	 C_Puts(msg);
	 G_LoadGameErr(msg);
	 free(msg);
	 return;
       }
   }

  save_p += sizeof checksum;
  while (*save_p++);

  for (i=0 ; i<MAXPLAYERS ; i++)
    playeringame[i] = *save_p++;
  save_p += MIN_MAXPLAYERS-MAXPLAYERS;         // killough 2/28/98

  // jff 3/17/98 restore idmus music
  // jff 3/18/98 account for unsigned byte
  // killough 11/98: simplify
  idmusnum = *(signed char *) save_p++;

  // load a base level
  // sf: in hubs, use g_doloadlevel instead of g_initnew
  if(hub_changelevel)
    G_DoLoadLevel();
  else
    G_InitNew(gameskill, gamemapname);

  // killough 3/1/98: Read game options
  // killough 11/98: move down to here
  save_p = G_ReadOptions(save_p);

  // get the times
  // killough 11/98: save entire word
  memcpy(&leveltime, save_p, sizeof save_p);
  save_p += sizeof save_p;

  // killough 11/98: load revenant tracer state
  basetic = gametic - (int) *save_p++;

  // dearchive all the modifications
  P_UnArchivePlayers();
  P_UnArchiveWorld();
  P_UnArchiveThinkers();
  P_UnArchiveSpecials();
  P_UnArchiveRNG();    // killough 1/18/98: load RNG information
  P_UnArchiveMap();    // killough 1/22/98: load automap information
  P_UnArchiveScripts(); // sf: scripting
  P_FreeObjTable();

  if (*save_p != 0xe6)
    {
      C_SetConsole();
      C_Printf("bad savegame: offset 0x%x is 0x%x\n",
	       save_p-savebuffer, *save_p);
      Z_Free(savebuffer);
      return; 
    }

  // done
  Z_Free(savebuffer);

  if (setsizeneeded)
    R_ExecuteSetViewSize();

  // draw the pattern into the back screen
  R_FillBackScreen();

  Z_CheckHeap();

  // killough 12/98: support -recordfrom and -loadgame -playdemo
  if (!command_loadgame)
    singledemo = false;         // Clear singledemo flag if loading from menu
  else
    if (singledemo)
      {
	gameaction = ga_loadgame; // Mark that we're loading a game before demo
	//	G_DoPlayDemo();           // This will detect it and won't reinit level
      }
    else       // Loading games from menu isn't allowed during demo recordings,
      if (demorecording) // So this can only possibly be a -recordfrom command.
	G_BeginRecording();// Start the -recordfrom, since the game was loaded.

}

static void G_DoLoadGame(void)
{
  G_LoadSavedLevel(savename);
}

//
// G_Ticker
// Make ticcmd_ts for the players.
//

void G_Ticker(void)
{
  int i;

  // do player reborns if needed
  for (i=0 ; i<MAXPLAYERS ; i++)
    if (playeringame[i] && players[i].playerstate == PST_REBORN)
      G_DoReborn (i);

  // do things to change the game state
  while (gameaction != ga_nothing)
    switch (gameaction)
      {
	case ga_loadlevel:
	  G_DoLoadLevel();
	  break;
	case ga_newgame:
	  G_DoNewGame();
	  break;
	case ga_loadgame:
	  G_DoLoadGame();
	  break;
	case ga_savegame:
	  G_DoSaveGame();
	  break;
	case ga_completed:
	  G_DoCompleted();
	  break;
	case ga_victory:
	  F_StartFinale();
	  break;
	case ga_worlddone:
	  G_DoWorldDone();
	  break;
	case ga_screenshot:
	  M_ScreenShot();
	  gameaction = ga_nothing;
	  break;
	case ga_loadhublevel:
	  P_DoChangeHubLevel();
	  break;
	case ga_playdemo:
	  CL_PlayDemo(demoname);
	  gameaction = ga_nothing;
	  break;
	case ga_timedemo:
	  CL_TimeDemo(demoname);
	  gameaction = ga_nothing;
	  break;
	default:  // killough 9/29/98
	  gameaction = ga_nothing;
	  break;
    }

  if(animscreenshot)    // animated screen shots
    {
      if(gametic % 16 == 0)
	{
	  animscreenshot--;
	  M_ScreenShot();
	}
    }

  // killough 10/6/98: allow games to be saved during demo
  // playback, by the playback user (not by demo itself)

  if (demoplayback && sendsave)
    {
      sendsave = false;
      G_DoSaveGame();
    }

  // killough 9/29/98: Skip some commands while pausing during demo
  // playback, or while menu is active.
  //
  // We increment basetic and skip processing if a demo being played
  // back is paused or if the menu is active while a non-net game is
  // being played, to maintain sync while allowing pauses.
  //
  // P_Ticker() does not stop netgames if a menu is activated, so
  // we do not need to stop if a menu is pulled up during netgames.

  if (paused & 2 || (!demoplayback && menuactive && !netgame))
    basetic++;  // For revenant tracers and RNG -- we must maintain sync
  else
    {
      for (i=0 ; i<MAXPLAYERS ; i++)
	{
	  if (playeringame[i])
	    {
	      ticcmd_t *cmd = &players[i].cmd;

	      memcpy(cmd, &player_cmds[i], sizeof *cmd);

              if(isconsoletic && netgame)
		continue;

	      // check for turbo cheats
	      // killough 2/14/98, 2/20/98 -- only warn in netgames and demos

	      if ((netgame || demoplayback) && 
		  cmd->forwardmove > TURBOTHRESHOLD &&
		  !(gametic&31) && ((gametic>>5)&3) == i )
		{
                  doom_printf("%s is turbo!", players[i].name); // killough 9/29/98
		}

	      // sf: merge special into the other loop

	      if (players[i].cmd.buttons & BT_SPECIAL)
                {
		  // killough 9/29/98: allow multiple special buttons
		  if (players[i].cmd.buttons & BTS_PAUSE)
                    {
                      paused ^= 1;
                      if (paused)
                        S_PauseSound();
                      else
                        S_ResumeSound();
                    }
		  
		  if (players[i].cmd.buttons & BTS_SAVEGAME)
		    {
		      if (!savedescription[0])
			strcpy(savedescription, "NET GAME");
		      savegameslot =
			(players[i].cmd.buttons & BTS_SAVEMASK)>>BTS_SAVESHIFT;
		      gameaction = ga_savegame;
		    }
                }
            }
        }
    }

  AM_Ticker();

  if((chasecam_active = camera==&chasecam))
    P_ChaseTicker();

  // cooldemo countdown
  
  if(demoplayback && cooldemo)
    {
      if(cooldemo_tics)
	cooldemo_tics--;
      else
	G_CoolViewPoint();                
    }

  DEBUGMSG("  g_ticker: main actions\n");

  // do main actions

  // killough 9/29/98: split up switch statement
  // into pauseable and unpauseable parts.

                // call other tickers
  C_NetTicker();        // sf: console network commands

  // sf: slightly more understandable,
  // killoughs system was pointlessly complex
  // obfuscation is useful but insane when taken to the extreme

  if(gamestate == GS_LEVEL)
    {                    
      P_Ticker();
      ST_Ticker(); 
      HU_Ticker();
    }
  else if(paused & 2 );
  else if(gamestate == GS_INTERMISSION) WI_Ticker();
  else if(gamestate == GS_FINALE) F_Ticker();
  else if(gamestate == GS_DEMOSCREEN) D_PageTicker();
}

//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//

//
// G_PlayerReborn
// Called after a player dies
// almost everything is cleared and initialized
//

void G_PlayerReborn(int player)
{
  player_t *p;
  int i;
  int frags[MAXPLAYERS];
  int totalfrags;
  int killcount;
  int itemcount;
  int secretcount;
  char playername[20];
  int playercolour;
  skin_t *playerskin;

  memcpy (frags, players[player].frags, sizeof frags);
  killcount = players[player].killcount;
  itemcount = players[player].itemcount;
  secretcount = players[player].secretcount;
  strcpy(playername, players[player].name);
  playercolour = players[player].colormap;
  totalfrags=players[player].totalfrags;
  playerskin=players[player].skin;

  p = &players[player];

  // killough 3/10/98,3/21/98: preserve cheats across idclev
  {
    int cheats = p->cheats;
    memset (p, 0, sizeof(*p));
    p->cheats = cheats;
  }

  memcpy(players[player].frags, frags, sizeof(players[player].frags));
  players[player].colormap = playercolour;
  strcpy(players[player].name, playername);
  players[player].killcount = killcount;
  players[player].itemcount = itemcount;
  players[player].secretcount = secretcount;
  players[player].totalfrags = totalfrags;
  players[player].skin=playerskin;

  p->usedown = p->attackdown = true;  // don't do anything immediately
  p->playerstate = PST_LIVE;
  p->health = initial_health;  // Ty 03/12/98 - use dehacked values
  p->readyweapon = p->pendingweapon = wp_pistol;
                // sf: different weapons owned
  memcpy(p->weaponowned, default_weaponowned, sizeof(p->weaponowned));
  p->weaponowned[wp_fist] = true;     // always fist and pistol
  p->weaponowned[wp_pistol] = true;

  p->ammo[am_clip] = initial_bullets; // Ty 03/12/98 - use dehacked values

  for (i=0 ; i<NUMAMMO ; i++)
    p->maxammo[i] = maxammo[i];
}

//
// G_CheckSpot
// Returns false if the player cannot be respawned
// at the given mapthing_t spot
// because something is occupying it
//

void P_SpawnPlayer(mapthing_t *mthing);

static boolean G_CheckSpot(int playernum, mapthing_t *mthing)
{
  fixed_t     x,y;
  subsector_t *ss;
  unsigned    an;
  mobj_t      *mo;
  int         i;

  if (!players[playernum].mo)
    {
      // first spawn of level, before corpses
      for (i=0 ; i<playernum ; i++)
	if (players[i].mo->x == mthing->x << FRACBITS
	    && players[i].mo->y == mthing->y << FRACBITS)
	  return false;
      return true;
    }

  x = mthing->x << FRACBITS;
  y = mthing->y << FRACBITS;

  // killough 4/2/98: fix bug where P_CheckPosition() uses a non-solid
  // corpse to detect collisions with other players in DM starts
  //
  // Old code:
  // if (!P_CheckPosition (players[playernum].mo, x, y))
  //    return false;

  players[playernum].mo->flags |=  MF_SOLID;
  i = P_CheckPosition(players[playernum].mo, x, y);
  players[playernum].mo->flags &= ~MF_SOLID;
  if (!i)
    return false;

  // flush an old corpse if needed
  // killough 2/8/98: make corpse queue have an adjustable limit
  // killough 8/1/98: Fix bugs causing strange crashes

  if (bodyquesize > 0)
    {
      static mobj_t **bodyque;
      static size_t queuesize;
      if (queuesize < bodyquesize)
	{
	  bodyque = realloc(bodyque, bodyquesize*sizeof*bodyque);
	  memset(bodyque+queuesize, 0, 
		 (bodyquesize-queuesize)*sizeof*bodyque);
	  queuesize = bodyquesize;
	}
      if (bodyqueslot >= bodyquesize) 
	P_RemoveMobj(bodyque[bodyqueslot % bodyquesize]); 
      bodyque[bodyqueslot++ % bodyquesize] = players[playernum].mo; 
    }
  else
    if (!bodyquesize)
      P_RemoveMobj(players[playernum].mo);

  // spawn a teleport fog
  ss = R_PointInSubsector (x,y);
  an = ( ANG45 * (mthing->angle/45) ) >> ANGLETOFINESHIFT;

  mo = P_SpawnMobj(x+20*finecosine[an], y+20*finesine[an],
		   ss->sector->floorheight, MT_TFOG);

  if (players[consoleplayer].viewz != 1)
    S_StartSound(mo, sfx_telept);  // don't start sound on first frame

  return true;
}

//
// G_DeathMatchSpawnPlayer
// Spawns a player at one of the random death match spots
// called at level load and each death
//

void G_DeathMatchSpawnPlayer(int playernum)
{
  int j, selections = deathmatch_p - deathmatchstarts;

  if (selections < MAXPLAYERS)
  {
    C_Printf("\aOnly %i deathmatch spots, %d required\n", selections, MAXPLAYERS);
    C_SetConsole();
    return;
  }

  for (j=0 ; j<20 ; j++)
    {
      int i = P_Random(pr_dmspawn) % selections;
      if (G_CheckSpot(playernum, &deathmatchstarts[i]) )
	{
	  deathmatchstarts[i].type = playernum+1;
	  P_SpawnPlayer (&deathmatchstarts[i]);
	  return;
	}
    }

  // no good spot, so the player will probably get stuck
  P_SpawnPlayer (&playerstarts[playernum]);
}

//
// G_DoReborn
//

void G_DoReborn(int playernum)
{
  hub_changelevel = false;

  if (!netgame)
    {
      // reload level from scratch
      // sf: use P_HubReborn, so that if in a hub, we restart from the
      // last time we entered this level
      // normal levels are unaffected
      P_HubReborn();
    }
  else
    {                               // respawn at the start
      int i;

      // first dissasociate the corpse
      players[playernum].mo->player = NULL;

      // spawn at random spot if in death match
      if (deathmatch)
	{
	  G_DeathMatchSpawnPlayer (playernum);
	  return;
	}

      if (G_CheckSpot (playernum, &playerstarts[playernum]) )
	{
	  P_SpawnPlayer (&playerstarts[playernum]);
	  return;
	}

      // try to spawn at one of the other players spots
      for (i=0 ; i<MAXPLAYERS ; i++)
	{
	  if (G_CheckSpot (playernum, &playerstarts[i]) )
	    {
	      playerstarts[i].type = playernum+1; // fake as other player
	      P_SpawnPlayer (&playerstarts[i]);
	      playerstarts[i].type = i+1;   // restore
	      return;
	    }
	  // he's going to be inside something.  Too bad.
	}
      P_SpawnPlayer (&playerstarts[playernum]);
    }
}

void G_ScreenShot(void)
{
  gameaction = ga_screenshot;
}

// DOOM Par Times
int pars[4][10] = {
  {0},
  {0,30,75,120,90,165,180,180,30,165},
  {0,90,90,90,120,90,360,240,30,170},
  {0,90,45,90,150,90,90,165,30,135}
};

// DOOM II Par Times
int cpars[34] = {
  30,90,120,120,90,150,120,120,270,90,  //  1-10
  210,150,150,150,210,150,420,150,210,150,  // 11-20
  240,150,180,150,150,300,330,420,300,180,  // 21-30
  120,30,30,30          // 31-34
};

//
// G_WorldDone
//

void G_WorldDone(void)
{
  gameaction = ga_worlddone;

  if (secretexit)
    players[consoleplayer].didsecret = true;

  if(info_intertext)
    F_StartFinale();
  else
    if (gamemode == commercial)
      {
	switch (gamemap)
	  {
	  case 15:
	  case 31:
	    if (!secretexit)
	      break;
	  case 6:
	  case 11:
	  case 20:
	  case 30:
	    F_StartFinale();
	    break;
	  }
      }
}

static skill_t d_skill;
static int     d_episode;
static int     d_map;
static char    d_mapname[10];

int G_GetMapForName(char *name)
{
  int episode, map;

  if(gamemode == commercial)
    {
      episode = 1;
      map = isMAPxy(name) ? 10 * (name[3]-'0') + (name[4]-'0') : 0;
      return map;
    }
  else
    {
      if(isExMy(name))
	{
	  episode = name[1] - '0';
	  map = name[3] - '0';
	}
      else
	{
	  episode = 1;
	  map = 0;
	}
      return (episode*10) + map;
    }
}

char *G_GetNameForMap(int episode, int map)
{
  static char levelname[10];
  if(gamemode == commercial)
    {
      sprintf(levelname, "MAP%02d", map);
    }
  else
    {
      sprintf(levelname, "E%iM%i", episode, map);
    }
  return levelname;
}

void G_DeferedInitNewNum(skill_t skill, int episode, int map)
{
  G_DeferedInitNew(skill, G_GetNameForMap(episode, map) );
}

void G_DeferedInitNew(skill_t skill, char *levelname)
{
  strncpy(d_mapname, levelname, 8);
  d_map = G_GetMapForName(levelname);

  if(gamemode != commercial)
    {
      d_episode = d_map / 10;
      d_map = d_map % 10;
    }
  else
    d_episode = 1;
  d_skill = skill;

  gameaction = ga_newgame;
}

#ifdef DOGS
// killough 7/19/98: Marine's best friend :)
static int G_GetHelpers(void)
{
  int j = M_CheckParm ("-dog");

  if (!j)
    j = M_CheckParm ("-dogs");
  return j ? j+1 < myargc ? atoi(myargv[j+1]) : 1 : default_dogs;
}
#endif

// killough 3/1/98: function to reload all the default parameter
// settings before a new game begins

void G_ReloadDefaults(void)
{
  // killough 3/1/98: Initialize options based on config file
  // (allows functions above to load different values for demos
  // and savegames without messing up defaults).

  weapon_recoil = default_weapon_recoil;    // weapon recoil

  variable_friction = allow_pushers = true;

  monsters_remember = default_monsters_remember;   // remember former enemies

  monster_infighting = default_monster_infighting; // killough 7/19/98

#ifdef DOGS
  dogs = netgame ? 0 : G_GetHelpers();             // killough 7/19/98
  dog_jumping = default_dog_jumping;
#endif

  distfriend = default_distfriend;                 // killough 8/8/98

  monster_backing = default_monster_backing;     // killough 9/8/98

  monster_avoid_hazards = default_monster_avoid_hazards; // killough 9/9/98

  monster_friction = default_monster_friction;     // killough 10/98

  help_friends = default_help_friends;             // killough 9/9/98

  autoaim = default_autoaim;

  monkeys = default_monkeys;

  bfgtype = default_bfgtype;               // killough 7/19/98

  // jff 1/24/98 reset play mode to command line spec'd version
  // killough 3/1/98: moved to here
  respawnparm = clrespawnparm;
  fastparm = clfastparm;
  nomonsters = clnomonsters;

  //jff 3/24/98 set startskill from defaultskill in config file, unless
  // it has already been set by a -skill parameter
  if (startskill==sk_none)
    startskill = (skill_t)(defaultskill-1);

  demoplayback = false;
  netdemo = false;

  // killough 2/21/98:
  memset(playeringame+1, 0, sizeof(*playeringame)*(MAXPLAYERS-1));

  consoleplayer = 0;

  compatibility = false;     // killough 10/98: replaced by comp[] vector
  memcpy(comp, default_comp, sizeof comp);

  demo_version = VERSION;     // killough 7/19/98: use this version's id

  // killough 3/31/98, 4/5/98: demo sync insurance
  demo_insurance = default_demo_insurance == 1;
  
  G_ScrambleRand();
}

         // sf: seperate function
void G_ScrambleRand()
{                            // killough 3/26/98: shuffle random seed
  // sf: simpler
  rngseed = time(NULL);
}

void G_DoNewGame (void)
{
  G_StopDemo();
  G_ReloadDefaults();            // killough 3/1/98
  P_ClearHubs();                 // sf: clear hubs when starting new game
  
  netgame = false;               // killough 3/29/98
  deathmatch = false;
  basetic = gametic;             // killough 9/29/98

  G_InitNew(d_skill, d_mapname);
  gameaction = ga_nothing;
}

// killough 4/10/98: New function to fix bug which caused Doom
// lockups when idclev was used in conjunction with -fast.

void G_SetFastParms(int fast_pending)
{
  static int fast = 0;            // remembers fast state
  int i;

  if (fast != fast_pending)       // only change if necessary
    {
      if ((fast = fast_pending))
	{
	  for (i=S_SARG_RUN1; i<=S_SARG_PAIN2; i++)
	    if (states[i].tics != 1 || demo_compatibility) // killough 4/10/98
	      states[i].tics >>= 1;  // don't change 1->0 since it causes cycles
	  mobjinfo[MT_BRUISERSHOT].speed = 20*FRACUNIT;
	  mobjinfo[MT_HEADSHOT].speed = 20*FRACUNIT;
	  mobjinfo[MT_TROOPSHOT].speed = 20*FRACUNIT;
	}
      else
	{
	  for (i=S_SARG_RUN1; i<=S_SARG_PAIN2; i++)
	    states[i].tics <<= 1;
	  mobjinfo[MT_BRUISERSHOT].speed = 15*FRACUNIT;
	  mobjinfo[MT_HEADSHOT].speed = 10*FRACUNIT;
	  mobjinfo[MT_TROOPSHOT].speed = 10*FRACUNIT;
	}
    }
}

//
// G_InitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, playeringame[] should be set.

void G_InitNewNum(skill_t skill, int episode, int map)
{
  G_InitNew(skill, G_GetNameForMap(episode, map) );
}

void G_InitNew(skill_t skill, char *name)
{
  int i;

  if (paused)
    {
      paused = false;
      S_ResumeSound();
    }

  hub_changelevel = false;  // sf
  P_ClearHubs();
  
  if (skill > sk_nightmare)
    skill = sk_nightmare;

  G_SetFastParms(fastparm || skill == sk_nightmare);  // killough 4/10/98

  M_ClearRandom();

  respawnmonsters = skill == sk_nightmare || respawnparm;

  // force players to be initialized upon first level load
  for (i=0 ; i<MAXPLAYERS ; i++)
    players[i].playerstate = PST_REBORN;

  usergame = true;                // will be set false if a demo
  paused = false;

  if(demoplayback)
    {
      netgame = false;
      displayplayer = consoleplayer = 0;
      P_ResetChasecam();      // sf: displayplayer changed
    }

  G_StopDemo();

  automapactive = false;
  gameskill = skill;

  if(gamemapname) free(gamemapname);    //sf
  gamemapname = strdup(name);
  G_SetGameMap();  // sf
  
  //jff 4/16/98 force marks on automap cleared every new level start
  AM_clearMarks();

  if (demo_version >= 203)
    M_LoadOptions();     // killough 11/98: read OPTIONS lump from wad

  G_StopDemo();

  G_DoLoadLevel();
}


// These functions are used to read and write game-specific options in demos
// and savegames so that demo sync is preserved and savegame restoration is
// complete. Not all options (for example "compatibility"), however, should
// be loaded and saved here. It is extremely important to use the same
// positions as before for the variables, so if one becomes obsolete, the
// byte(s) should still be skipped over or padded with 0's.
// Lee Killough 3/1/98

byte *G_WriteOptions(byte *demo_p)
{
  byte *target = demo_p + GAME_OPTION_SIZE;

  *demo_p++ = monsters_remember;  // part of monster AI

  *demo_p++ = variable_friction;  // ice & mud

  *demo_p++ = weapon_recoil;      // weapon recoil

  *demo_p++ = allow_pushers;      // MT_PUSH Things

  *demo_p++ = 0;

  *demo_p++ = player_bobbing;  // whether player bobs or not

  // killough 3/6/98: add parameters to savegame, move around some in demos
  *demo_p++ = respawnparm;
  *demo_p++ = fastparm;
  *demo_p++ = nomonsters;

  *demo_p++ = demo_insurance;        // killough 3/31/98

  // killough 3/26/98: Added rngseed. 3/31/98: moved here
  *demo_p++ = (rngseed >> 24) & 0xff;
  *demo_p++ = (rngseed >> 16) & 0xff;
  *demo_p++ = (rngseed >>  8) & 0xff;
  *demo_p++ =  rngseed        & 0xff;

  // Options new to v2.03 begin here

  *demo_p++ = monster_infighting;   // killough 7/19/98

#ifdef DOGS
  *demo_p++ = dogs;                 // killough 7/19/98
#else
  *demo_p++ = 0;
#endif

  *demo_p++ = bfgtype;          // killough 7/19/98

  demo_p++ ; //sf: remove beta emulation but keep space

  *demo_p++ = (distfriend >> 8) & 0xff;  // killough 8/8/98  
  *demo_p++ =  distfriend       & 0xff;  // killough 8/8/98  

  *demo_p++ = monster_backing;         // killough 9/8/98

  *demo_p++ = monster_avoid_hazards;    // killough 9/9/98

  *demo_p++ = monster_friction;         // killough 10/98

  *demo_p++ = help_friends;             // killough 9/9/98

#ifdef DOGS
  *demo_p++ = dog_jumping;
#else
  *demo_p++ = 0;
#endif

  *demo_p++ = monkeys;

  {   // killough 10/98: a compatibility vector now
    int i;
    for (i=0; i < COMP_TOTAL; i++)
      *demo_p++ = comp[i] != 0;
  }

  //----------------
  // Padding at end
  //----------------
  while (demo_p < target)
    *demo_p++ = 0;

  if (demo_p != target)
    I_Error("G_WriteOptions: GAME_OPTION_SIZE is too small");

  return target;
}

// Same, but read instead of write

byte *G_ReadOptions(byte *demo_p)
{
  byte *target = demo_p + GAME_OPTION_SIZE;

  monsters_remember = *demo_p++;

  variable_friction = *demo_p;  // ice & mud
  demo_p++;

  weapon_recoil = *demo_p;       // weapon recoil
  demo_p++;

  allow_pushers = *demo_p;      // MT_PUSH Things
  demo_p++;

  demo_p++;

        //sf: remove player bobbing
//  player_bobbing = *demo_p;     // whether player bobs or not
  demo_p++;

  // killough 3/6/98: add parameters to savegame, move from demo
  respawnparm = *demo_p++;
  fastparm = *demo_p++;
  nomonsters = *demo_p++;

  demo_insurance = *demo_p++;              // killough 3/31/98

  // killough 3/26/98: Added rngseed to demos; 3/31/98: moved here

  rngseed  = *demo_p++ & 0xff;
  rngseed <<= 8;
  rngseed += *demo_p++ & 0xff;
  rngseed <<= 8;
  rngseed += *demo_p++ & 0xff;
  rngseed <<= 8;
  rngseed += *demo_p++ & 0xff;

  // Options new to v2.03
  if (demo_version >= 203)
    {
      monster_infighting = *demo_p++;   // killough 7/19/98

#ifdef DOGS
      dogs = *demo_p++;                 // killough 7/19/98
#else
      demo_p++;
#endif

      bfgtype = *demo_p++;          // killough 7/19/98
      demo_p ++;        // sf: where beta was

      distfriend = *demo_p++ << 8;      // killough 8/8/98
      distfriend+= *demo_p++;

      monster_backing = *demo_p++;     // killough 9/8/98

      monster_avoid_hazards = *demo_p++; // killough 9/9/98

      monster_friction = *demo_p++;      // killough 10/98

      help_friends = *demo_p++;          // killough 9/9/98

#ifdef DOGS
      dog_jumping = *demo_p++;           // killough 10/98
#else
      demo_p++;
#endif

      monkeys = *demo_p++;

      {   // killough 10/98: a compatibility vector now
	int i;
	for (i=0; i < COMP_TOTAL; i++)
	  comp[i] = *demo_p++;
      }

      // Options new to v2.04, etc.
      if (demo_version >= 204)
	;
    }
  else  // defaults for versions < 2.02
    {
      int i;  // killough 10/98: a compatibility vector now
      for (i=0; i < COMP_TOTAL; i++)
	comp[i] = compatibility;

      monster_infighting = 1;           // killough 7/19/98

      monster_backing = 0;              // killough 9/8/98
      
      monster_avoid_hazards = 0;        // killough 9/9/98

      monster_friction = 0;             // killough 10/98

      help_friends = 0;                 // killough 9/9/98

      bfgtype = bfg_normal;                  // killough 7/19/98

#ifdef DOGS
      dogs = 0;                         // killough 7/19/98
      dog_jumping = 0;                  // killough 10/98
#endif
      monkeys = 0;

      default_autoaim = autoaim;
      autoaim = 1;
    }

  return target;
}

void G_BeginRecording(void)
{
  int i;

  demo_p = demobuffer;

  *demo_p++ = VERSION;

  // signature
  *demo_p++ = 0x1d;
  *demo_p++ = 'M';
  *demo_p++ = 'B';
  *demo_p++ = 'F';
  *demo_p++ = 0xe6;
  *demo_p++ = '\0';

  // killough 2/22/98: save compatibility flag in new demos
  *demo_p++ = compatibility;       // killough 2/22/98

  demo_version = VERSION;     // killough 7/19/98: use this version's id

  *demo_p++ = gameskill;
  *demo_p++ = gameepisode;
  *demo_p++ = gamemap;
  *demo_p++ = deathmatch;
  *demo_p++ = consoleplayer;

  demo_p = G_WriteOptions(demo_p); // killough 3/1/98: Save game options

  for (i=0 ; i<MAXPLAYERS ; i++)
    *demo_p++ = playeringame[i];

  // killough 2/28/98:
  // We always store at least MIN_MAXPLAYERS bytes in demo, to
  // support enhancements later w/o losing demo compatibility

  for (; i<MIN_MAXPLAYERS; i++)
    *demo_p++ = 0;
}


// killough 1/22/98: this is a "Doom printf" for messages. I've gotten
// tired of using players->message=... and so I've added this doom_printf.
//
// killough 3/6/98: Made limit static to allow z_zone functions to call
// this function, without calling realloc(), which seems to cause problems.

// sf: changed to run console command instead

#define MAX_MESSAGE_SIZE 1024

void doom_printf(const char *s, ...)
{
  static char msg[MAX_MESSAGE_SIZE];
  va_list v;
  va_start(v,s);
  vsprintf(msg,s,v);                  // print message in buffer
  va_end(v);
  C_Puts(msg);  // set new message
  HU_PlayerMsg(msg);
}

        // sf: printf to a particular player only
        // to make up for the loss of player->msg = ...

void player_printf(player_t *player, const char *s, ...)
{
  static char msg[MAX_MESSAGE_SIZE];
  va_list v;
  va_start(v,s);
  vsprintf(msg,s,v);                  // print message in buffer
  va_end(v);

  if(player == &players[consoleplayer])
    doom_printf(msg);
}


extern int numcameraviews;      // wi_stuff.c
extern camera_t intercam;
extern mapthing_t *camerathing[MAXCAMERAS];

        // cool demo: change to new viewpoint
void G_CoolViewPoint()
{
  // 2 if no cameras, 3 if cameras
  int viewtype = M_Random() % (2 + !!numcameraviews);
  int old_displayplayer = displayplayer;
  
  // pick the next player
  do
    { 
      displayplayer++; 
      if (displayplayer == MAXPLAYERS) 
	displayplayer = 0; 
    } while (!playeringame[displayplayer]);
  
  if(displayplayer != old_displayplayer)
    {
      ST_Start();
      HU_Start();
      S_UpdateSounds(players[displayplayer].mo);
      P_ResetChasecam();      // reset the chasecam
    }
  
  // turn off the chasecam?
  if(chasecam_active && viewtype != 1)
    {
      chasecam_active = false;
      P_ChaseEnd();
    }
  
  if(viewtype == 0)       // normal player-view
    {
    }
  else if(viewtype == 1)  // view from the chasecam
    {
      chasecam_active = true;
      P_ChaseStart();
    }
  else if(viewtype == 2) // camera view
    {
      int cam = M_Random() % numcameraviews;
      
      P_ResetChasecam();      // turn off the chasecam
      // if its still on
      
      intercam.x = camerathing[cam]->x*FRACUNIT;
      intercam.y = camerathing[cam]->y*FRACUNIT;
      intercam.angle = R_WadToAngle(camerathing[cam]->angle);
      intercam.updownangle = 0;
      intercam.z = R_PointInSubsector(intercam.x, intercam.y)
	->sector->floorheight + 41*FRACUNIT;
      camera = &intercam;
    }
  
  // pic a random number of tics until changing the viewpoint
  cooldemo_tics = (7 + M_Random() % 13) * 35;
}


//-----------------------------------------------------------------------------
//
// $Log$
// Revision 1.3  2001-01-18 01:35:52  fraggle
// fix up demo code somewhat
//
// Revision 1.2  2000/05/22 10:01:54  fraggle
// ctrl-d to disconnect from server
//
// Revision 1.1.1.1  2000/04/30 19:12:08  fraggle
// initial import
//
//
//-----------------------------------------------------------------------------
