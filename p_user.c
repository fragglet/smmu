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
// DESCRIPTION:
//      Player related stuff.
//      Bobbing POV/weapon, movement.
//      Pending weapon.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id$";

#include "doomstat.h"
#include "d_event.h"
#include "c_net.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "r_main.h"
#include "p_map.h"
#include "p_maputl.h"
#include "p_pspr.h"
#include "p_spec.h"
#include "p_user.h"

//
// Netgame prediction variables
//

boolean predicted_tic;             // true if we are running a predicted tic

// Index of the special effects (INVUL inverse) map.

#define INVERSECOLORMAP 32

//
// Movement.
//

// 16 pixels of bob

#define MAXBOB  0x100000

boolean onground; // whether player is on ground or in air

//
// P_Thrust
// Moves the given origin along a given angle.
//

void P_Thrust(player_t* player,angle_t angle,fixed_t move)
{
  player->mo->momx += FixedMul(move,finecosine[angle >>= ANGLETOFINESHIFT]);
  player->mo->momy += FixedMul(move,finesine[angle]);
}

//
// P_Bob
// Same as P_Thrust, but only affects bobbing.
//
// killough 10/98: We apply thrust separately between the real physical player
// and the part which affects bobbing. This way, bobbing only comes from player
// motion, nothing external, avoiding many problems, e.g. bobbing should not
// occur on conveyors, unless the player walks on one, and bobbing should be
// reduced at a regular rate, even on ice (where the player coasts).
//

void P_Bob(player_t *player, angle_t angle, fixed_t move)
{
  player->momx += FixedMul(move,finecosine[angle >>= ANGLETOFINESHIFT]);
  player->momy += FixedMul(move,finesine[angle]);
}

//
// P_CalcHeight
// Calculate the walking / running height adjustment
//

void P_CalcHeight (player_t* player)
{
  int     angle;
  fixed_t bob;

  // Regular movement bobbing
  // (needs to be calculated for gun swing
  // even if not on ground)
  // OPTIMIZE: tablify angle
  // Note: a LUT allows for effects
  //  like a ramp with low health.

  // killough 10/98: Make bobbing depend only on player-applied motion.
  //
  // Note: don't reduce bobbing here if on ice: if you reduce bobbing here,
  // it causes bobbing jerkiness when the player moves from ice to non-ice,
  // and vice-versa.

  player->bob = player_bobbing ? (FixedMul(player->momx,player->momx) + 
				  FixedMul(player->momy,player->momy))>>2 : 0;

  if (player->bob > MAXBOB)
    player->bob = MAXBOB;

  if (!onground || player->cheats & CF_NOMOMENTUM)
    {
      player->viewz = player->mo->z + VIEWHEIGHT;

      if (player->viewz > player->mo->ceilingz-4*FRACUNIT)
	player->viewz = player->mo->ceilingz-4*FRACUNIT;

      // phares 2/25/98:
      // The following line was in the Id source and appears
      // to be a bug. player->viewz is checked in a similar
      // manner at a different exit below.

      // player->viewz = player->mo->z + player->viewheight;

      return;
    }

  // sf: use player->bobtime rather than leveltime
  // stop shakyness when using player prediction
  
  angle = (FINEANGLES/20*player->bobtime)&FINEMASK;
  bob = FixedMul(player->bob/2,finesine[angle]);

  player->bobtime++;
  
  // move viewheight

  if (player->playerstate == PST_LIVE)
    {
      player->viewheight += player->deltaviewheight;

      if (player->viewheight > VIEWHEIGHT)
	{
	  player->viewheight = VIEWHEIGHT;
	  player->deltaviewheight = 0;
	}

      if (player->viewheight < VIEWHEIGHT/2)
	{
	  player->viewheight = VIEWHEIGHT/2;
	  if (player->deltaviewheight <= 0)
	    player->deltaviewheight = 1;
	}

      if (player->deltaviewheight)
	{
	  player->deltaviewheight += FRACUNIT/4;
	  if (!player->deltaviewheight)
	    player->deltaviewheight = 1;
	}
    }

  player->viewz = player->mo->z + player->viewheight + bob;

  if (player->viewz > player->mo->ceilingz-4*FRACUNIT)
    player->viewz = player->mo->ceilingz-4*FRACUNIT;
}

//
// P_MovePlayer
//
// Adds momentum if the player is not in the air
//
// killough 10/98: simplified

void P_MovePlayer (player_t* player)
{
  ticcmd_t *cmd = &player->cmd;
  mobj_t *mo = player->mo;

  mo->angle += cmd->angleturn << 16;
  onground = mo->z <= mo->floorz;

  // killough 10/98:
  //
  // We must apply thrust to the player and bobbing separately, to avoid
  // anomalies. The thrust applied to bobbing is always the same strength on
  // ice, because the player still "works just as hard" to move, while the
  // thrust applied to the movement varies with 'movefactor'.

  if (cmd->forwardmove | cmd->sidemove) // killough 10/98
    {
      if (onground || mo->flags & MF_BOUNCES) // killough 8/9/98
	{
	  int friction, movefactor = P_GetMoveFactor(mo, &friction);

	  // killough 11/98:
	  // On sludge, make bobbing depend on efficiency.
	  // On ice, make it depend on effort.

	  int bobfactor =
	    friction < ORIG_FRICTION ? movefactor : ORIG_FRICTION_FACTOR;

	  if (cmd->forwardmove)
	    {
	      P_Bob(player,mo->angle,cmd->forwardmove*bobfactor);
	      P_Thrust(player,mo->angle,cmd->forwardmove*movefactor);
	    }

	  if (cmd->sidemove)
	    {
	      P_Bob(player,mo->angle-ANG90,cmd->sidemove*bobfactor);
	      P_Thrust(player,mo->angle-ANG90,cmd->sidemove*movefactor);
	    }
	}
      if (mo->state == states+S_PLAY)
	P_SetMobjState(mo,S_PLAY_RUN1);
    }
}

#define ANG5 (ANG90/18)

//
// P_DeathThink
// Fall on your face when dying.
// Decrease POV height to floor height.
//

void P_DeathThink (player_t* player)
{
  angle_t angle;
  angle_t delta;

  P_MovePsprites (player);

  // fall to the ground

  if (player->viewheight > 6*FRACUNIT)
    player->viewheight -= FRACUNIT;

  if (player->viewheight < 6*FRACUNIT)
    player->viewheight = 6*FRACUNIT;

  player->deltaviewheight = 0;
  onground = (player->mo->z <= player->mo->floorz);
  P_CalcHeight (player);

  if (player->attacker && player->attacker != player->mo)
    {
      angle = R_PointToAngle2 (player->mo->x,
			       player->mo->y,
			       player->attacker->x,
			       player->attacker->y);

      delta = angle - player->mo->angle;

      if (delta < ANG5 || delta > (unsigned)-ANG5)
	{
	  // Looking at killer,
	  //  so fade damage flash down.

	  player->mo->angle = angle;

	  if (player->damagecount)
	    player->damagecount--;
	}
      else 
	if (delta < ANG180)
	  player->mo->angle += ANG5;
	else
	  player->mo->angle -= ANG5;
    }
  else 
    if (player->damagecount)
      player->damagecount--;

  if (player->cmd.buttons & BT_USE)
    player->playerstate = PST_REBORN;
}

//
// P_PlayerThink
//

void P_PlayerThink (player_t* player)
{
  ticcmd_t*    cmd;
  weapontype_t newweapon;

  // killough 2/8/98, 3/21/98:
  // (this code is necessary despite questions raised elsewhere in a comment)

  if (player->cheats & CF_NOCLIP)
    player->mo->flags |= MF_NOCLIP;
  else
    player->mo->flags &= ~MF_NOCLIP;

  // chain saw run forward

  cmd = &player->cmd;
  if (player->mo->flags & MF_JUSTATTACKED)
    {
      cmd->angleturn = 0;
      cmd->forwardmove = 0xc800/512;
      cmd->sidemove = 0;
      player->mo->flags &= ~MF_JUSTATTACKED;
    }

  if (player->playerstate == PST_DEAD)
    {
      P_DeathThink (player);
      return;
    }

  // Move around.
  // Reactiontime is used to prevent movement
  //  for a bit after a teleport.

  if (player->mo->reactiontime)
    player->mo->reactiontime--;
  else
    {
      P_MovePlayer (player);
      if (cmd->updownangle)      // wait til teleport finishes to look around
	player->updownangle += cmd->updownangle;
    }

        // looking up/down checks
  if(player->updownangle < -50) player->updownangle = -50;
  if(player->updownangle > 50) player->updownangle = 50;
  if(!allowmlook) player->updownangle = 0;

  if(player->readyweapon == wp_bfg)
    {
      if(bfglook == 0) player->updownangle = 0;
      if(bfglook == 2 && player->updownangle < -10)
	player->updownangle = -10;
    }
  
  P_CalcHeight (player); // Determines view height and bobbing

  // Determine if there's anything about the sector you're in that's
  // going to affect you, like painful floors.

  if (player->mo->subsector->sector->special)
    P_PlayerInSpecialSector (player);

  // Sprite Height problem...                                         // phares
  // Future code:                                                     //  |
  // It's possible that at this point the player is standing on top   //  V
  // of a Thing that could cause him some damage, like a torch or
  // burning barrel. We need a way to generalize Thing damage by
  // grabbing a bit in the Thing's options to indicate damage. Since
  // this is competing with other attributes we may want to add,
  // we'll put this off for future consideration when more is
  // known.

  // Future Code:                                                     //  ^
  // Check to see if the object you've been standing on has moved     //  |
  // out from underneath you.                                         // phares

  // Check for weapon change.

  // A special event has no other buttons.

  if (cmd->buttons & BT_SPECIAL)
    cmd->buttons = 0;

  if (cmd->buttons & BT_CHANGE)
    {
      // The actual changing of the weapon is done
      //  when the weapon psprite can do it
      //  (read: not in the middle of an attack).

      newweapon = (cmd->buttons & BT_WEAPONMASK)>>BT_WEAPONSHIFT;

      // killough 3/22/98: For demo compatibility we must perform the fist
      // and SSG weapons switches here, rather than in G_BuildTiccmd(). For
      // other games which rely on user preferences, we must use the latter.

      if (demo_compatibility)
	{ // compatibility mode -- required for old demos -- killough
	  if (newweapon == wp_fist && player->weaponowned[wp_chainsaw] &&
	      (player->readyweapon != wp_chainsaw ||
	       !player->powers[pw_strength]))
	    newweapon = wp_chainsaw;
	  if (gamemode == commercial &&
	      newweapon == wp_shotgun &&
	      player->weaponowned[wp_supershotgun] &&
	      player->readyweapon != wp_supershotgun)
	    newweapon = wp_supershotgun;
	}

      // killough 2/8/98, 3/22/98 -- end of weapon selection changes

      if (player->weaponowned[newweapon] && newweapon != player->readyweapon)

	// Do not go to plasma or BFG in shareware,
	//  even if cheated.

	if ((newweapon != wp_plasma && newweapon != wp_bfg)
	    || (gamemode != shareware) )
	  player->pendingweapon = newweapon;
    }

  // check for use

  if (cmd->buttons & BT_USE)
    {
      if (!player->usedown)
	{
	  P_UseLines (player);
	  player->usedown = true;
	}
    }
  else
    player->usedown = false;

  // cycle psprites
  // sf: dont try to predict psprites
  
  if(!predicted_tic)
    P_MovePsprites (player);

  // Counters, time dependent power ups.

  // Strength counts up to diminish fade.

  if (player->powers[pw_strength])
    player->powers[pw_strength]++;

  // killough 1/98: Make idbeholdx toggle:

  if (player->powers[pw_invulnerability] > 0) // killough
    player->powers[pw_invulnerability]--;

  if(player->powers[pw_invisibility] > 0)
    if (! --player->powers[pw_invisibility] )
       player->mo->flags &= ~MF_SHADOW;

  if (player->powers[pw_infrared] > 0)        // killough
    player->powers[pw_infrared]--;

  if (player->powers[pw_ironfeet] > 0)        // killough
    player->powers[pw_ironfeet]--;

  if (player->damagecount)
    player->damagecount--;

  if (player->bonuscount)
    player->bonuscount--;

  // Handling colormaps.
  // killough 3/20/98: reformat to terse C syntax

  player->fixedcolormap = 

    // sf: removed MBF beta stuff

  player->powers[pw_invulnerability] > 4*32 ||    /* Regular Doom */
  player->powers[pw_invulnerability] & 8 ? INVERSECOLORMAP :
  player->powers[pw_infrared] > 4*32 || player->powers[pw_infrared] & 8;
}

//============================================================================
//
// Player Movement Prediction
//
// In netgames, we can often get stuck waiting for the other player to send
// their tics - this results in 'lag' where the buttons we press and
// movements that we make do not occur until a few tics after we made them.
// We use player movement prediction to move our viewpoint while we still
// wait for the other players to send their movements.
//
//============================================================================

void A_WeaponReady();

// predicted player and player object -- copy of the actual ones

static player_t *actual_player;
static player_t predicted_player;
static mobj_t predicted_mobj;

void P_StartPrediction(player_t *player)
{
  actual_player = player;
  
  // copy the player and player->mo to
  // predicted_player and predicted_mobj
  // do not let predicted_mobj pick up objects (which could mess up sync)
  
  memcpy(&predicted_player, player, sizeof(*player));
  memcpy(&predicted_mobj, player->mo, sizeof(*player->mo));
  predicted_mobj.flags &= ~MF_PICKUP;

  // link predicted_player and predicted_mobj

  predicted_player.mo = &predicted_mobj;
  predicted_mobj.player= &predicted_player;

  player->predicted = &predicted_player;
}

// predict psprites

static void PredictPsprites()
{
  int i;
  
  for(i=0; i<NUMPSPRITES; i++)
    {
      pspdef_t *psp = &predicted_player.psprites[i];

      if(psp->state && psp->state->action == A_WeaponReady)
	{
	  // bob the weapon based on movement speed
	  
	  int angle = (128*leveltime) & FINEMASK;
	  psp->sx = FRACUNIT +
	    FixedMul(predicted_player.bob, finecosine[angle]);
	  angle &= FINEANGLES/2-1;
	  psp->sy = WEAPONTOP +
	    FixedMul(predicted_player.bob, finesine[angle]);
	}
    }
}

void P_RunPredictedTic(ticcmd_t *ticcmd)
{
  // unhook the actual player object from the level
  // hook predicted_mobj into level

  P_UnsetThingPosition(actual_player->mo);
  P_SetThingPosition(&predicted_mobj);
  
  // run ticcmd

  predicted_tic = true;
  
  memcpy(&predicted_player.cmd, ticcmd, sizeof(*ticcmd));
  predicted_player.cmd.buttons = 0;

  P_PlayerThink (&predicted_player);
  P_MobjThinker(&predicted_mobj);
  PredictPsprites();
  
  // sf: run heads up - make sure we update the lightup crosshair

  HU_Ticker();
  
  predicted_tic = false;    // back to reality
  
  // unhook predicted_mobj and hook the actual player obj back in

  P_UnsetThingPosition(&predicted_mobj);
  P_SetThingPosition(actual_player->mo);
}


//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.6  2000-06-20 21:06:10  fraggle
// predict psprite bob
//
// Revision 1.5  2000/05/24 13:29:11  fraggle
// fix jerkiness problem w/client prediction
//
// Revision 1.4  2000/05/22 10:14:02  fraggle
// nothing
//
// Revision 1.3  2000/05/07 13:01:12  fraggle
// lightup crosshair prediction
//
// Revision 1.2  2000/05/02 15:43:41  fraggle
// client movement prediction
//
// Revision 1.1.1.1  2000/04/30 19:12:08  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
