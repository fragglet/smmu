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
//      Moving object handling. Spawn functions.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id$";

#include "doomdef.h"
#include "doomstat.h"
#include "m_random.h"
#include "r_main.h"
#include "p_maputl.h"
#include "p_map.h"
#include "p_tick.h"
#include "sounds.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "s_sound.h"
#include "info.h"
#include "g_game.h"
#include "p_chase.h"
#include "p_inter.h"
#include "p_user.h"
#include "wi_stuff.h"

int gravity=FRACUNIT;

//
// P_SetMobjState
// Returns true if the mobj is still present.
//

boolean P_SetMobjState(mobj_t* mobj,statenum_t state)
{
  state_t*  st;

  // killough 4/9/98: remember states seen, to detect cycles:

  static statenum_t seenstate_tab[NUMSTATES]; // fast transition table
  statenum_t *seenstate = seenstate_tab;      // pointer to table
  static int recursion;                       // detects recursion
  statenum_t i = state;                       // initial state
  boolean ret = true;                         // return value
  statenum_t tempstate[NUMSTATES];            // for use with recursion

  if (recursion++)                            // if recursion detected,
    memset(seenstate=tempstate,0,sizeof tempstate); // clear state table

  do
    {
      if (state == S_NULL)
	{
	  mobj->state = (state_t *) S_NULL;
	  P_RemoveMobj (mobj);
	  ret = false;
	  break;                 // killough 4/9/98
	}

      st = states + state;
      mobj->state = st;
      mobj->tics = st->tics;

      // sf: skins
      mobj->sprite = mobj->skin ? mobj->skin->sprite : st->sprite;

      mobj->frame = st->frame;

      // Modified handling.
      // Call action functions when the state is set
      // sf: not if running predicted tic
      
      if (!predicted_tic && st->action)
	st->action(mobj);

      seenstate[state] = 1 + st->nextstate;   // killough 4/9/98

      state = st->nextstate;
    } 
  while (!mobj->tics && !seenstate[state]);   // killough 4/9/98

  if (ret && !mobj->tics)  // killough 4/9/98: detect state cycles
    doom_printf("Warning: State Cycle Detected");

  if (!--recursion)
    for (;(state=seenstate[i]);i=state-1)
      seenstate[i] = 0;  // killough 4/9/98: erase memory of states

  return ret;
}

//
// P_ExplodeMissile
//

void P_ExplodeMissile (mobj_t* mo)
{
  mo->momx = mo->momy = mo->momz = 0;

  P_SetMobjState(mo, mobjinfo[mo->type].deathstate);

  mo->tics -= P_Random(pr_explode)&3;

  if (mo->tics < 1)
    mo->tics = 1;

  mo->flags &= ~MF_MISSILE;

  if (mo->info->deathsound)
    S_StartSound (mo, mo->info->deathsound);
}

extern mobj_t *hitthing;

//
// P_XYMovement
//
// Attempts to move something if it has momentum.
//
// killough 11/98: minor restructuring

void P_XYMovement (mobj_t* mo)
{
  player_t *player;
  fixed_t xmove, ymove;

  if (!(mo->momx | mo->momy)) // Any momentum?
    {
      if (mo->flags & MF_SKULLFLY)
	{
	  // the skull slammed into something

	  mo->flags &= ~MF_SKULLFLY;
	  mo->momz = 0;

	  P_SetMobjState(mo, mo->info->spawnstate);
	}
      return;
    }

  player = mo->player;

  if (mo->momx > MAXMOVE)
    mo->momx = MAXMOVE;
  else 
    if (mo->momx < -MAXMOVE)
      mo->momx = -MAXMOVE;

  if (mo->momy > MAXMOVE)
    mo->momy = MAXMOVE;
  else
    if (mo->momy < -MAXMOVE)
      mo->momy = -MAXMOVE;

  xmove = mo->momx;
  ymove = mo->momy;

  do
    {
      fixed_t ptryx, ptryy;

      // killough 8/9/98: fix bug in original Doom source:
      // Large negative displacements were never considered.
      // This explains the tendency for Mancubus fireballs
      // to pass through walls.

      if (xmove > MAXMOVE/2 || ymove > MAXMOVE/2 ||  // killough 8/9/98:
	  ((xmove < -MAXMOVE/2 || ymove < -MAXMOVE/2) && demo_version >= 203))
	{
	  ptryx = mo->x + xmove/2;
	  ptryy = mo->y + ymove/2;
	  xmove >>= 1;
	  ymove >>= 1;
	}
      else
	{
	  ptryx = mo->x + xmove;
	  ptryy = mo->y + ymove;
	  xmove = ymove = 0;
	}

      // killough 3/15/98: Allow objects to drop off

      if (!P_TryMove(mo, ptryx, ptryy, true))
	{
	  // blocked move

	  // killough 8/11/98: bouncing off walls
	  // killough 10/98:
	  // Add ability for objects other than players to bounce on ice
	  
	  if (!(mo->flags & MF_MISSILE) && demo_version >= 203 &&
	      (mo->flags & MF_BOUNCES || 
	       (!player && blockline &&
		variable_friction && mo->z <= mo->floorz &&
		P_GetFriction(mo, NULL) > ORIG_FRICTION)))
	    {
	      if (blockline)
		{
		  fixed_t r = ((blockline->dx >> FRACBITS) * mo->momx +
			       (blockline->dy >> FRACBITS) * mo->momy) /
		    ((blockline->dx >> FRACBITS)*(blockline->dx >> FRACBITS)+
		     (blockline->dy >> FRACBITS)*(blockline->dy >> FRACBITS));
		  fixed_t x = FixedMul(r, blockline->dx);
		  fixed_t y = FixedMul(r, blockline->dy);

		  // reflect momentum away from wall

		  mo->momx = x*2 - mo->momx;
		  mo->momy = y*2 - mo->momy;

		  // if under gravity, slow down in
		  // direction perpendicular to wall.

		  if (!(mo->flags & MF_NOGRAVITY))
		    {
		      mo->momx = (mo->momx + x)/2;
		      mo->momy = (mo->momy + y)/2;
		    }
		}
	      else
              {
		mo->momx = mo->momy = 0;
              }
	    }
	  else
	    if (player)   // try to slide along it
	      P_SlideMove (mo);
	    else 
	      if (mo->flags & MF_MISSILE)
		{
		  // explode a missile

		  if (ceilingline &&
		      ceilingline->backsector &&
		      ceilingline->backsector->ceilingpic == skyflatnum)
		    if (demo_compatibility ||  // killough
			mo->z > ceilingline->backsector->ceilingheight)
		      {
			// Hack to prevent missiles exploding
			// against the sky.
			// Does not handle sky floors.

			P_RemoveMobj (mo);
			return;
		      }
		  P_ExplodeMissile (mo);
		}
	      else // whatever else it is, it is now standing still in (x,y)
              {
                mo->momx = mo->momy = 0;
              }
	}
    }
  while (xmove | ymove);

  // slow down

#if 0  
  // killough 10/98: this is unused code (except maybe in .deh files?)
  if (player && player->mo == mo && player->cheats & CF_NOMOMENTUM)
    {
      // debug option for no sliding at all
      mo->momx = mo->momy = 0;
      player->momx = player->momy = 0;         // killough 10/98
      return;
    }
#endif

  // no friction for missiles or skulls ever, no friction when airborne
  if (mo->flags & (MF_MISSILE | MF_SKULLFLY) || mo->z > mo->floorz)
    return;

  // killough 8/11/98: add bouncers
  // killough 9/15/98: add objects falling off ledges
  // killough 11/98: only include bouncers hanging off ledges
  if (((mo->flags & MF_BOUNCES && mo->z > mo->dropoffz) ||
       mo->flags & MF_CORPSE || mo->intflags & MIF_FALLING) &&
      (mo->momx > FRACUNIT/4 || mo->momx < -FRACUNIT/4 ||
       mo->momy > FRACUNIT/4 || mo->momy < -FRACUNIT/4) &&
      mo->floorz != mo->subsector->sector->floorheight)
    return;  // do not stop sliding if halfway off a step with some momentum

  // killough 11/98:
  // Stop voodoo dolls that have come to rest, despite any
  // moving corresponding player, except in old demos:

  if (mo->momx > -STOPSPEED && mo->momx < STOPSPEED &&
      mo->momy > -STOPSPEED && mo->momy < STOPSPEED &&
      (!player || !(player->cmd.forwardmove | player->cmd.sidemove) ||
       (player->mo != mo && demo_version >= 203)))
    {
      // if in a walking frame, stop moving

      // killough 10/98:
      // Don't affect main player when voodoo dolls stop, except in old demos:

      if (player && (unsigned)(player->mo->state - states - S_PLAY_RUN1) < 4 
	  && (player->mo == mo || demo_version < 203))
	P_SetMobjState(player->mo, S_PLAY);

      mo->momx = mo->momy = 0;
      
      // killough 10/98: kill any bobbing momentum too (except in voodoo dolls)
      if (player && player->mo == mo)
	player->momx = player->momy = 0; 
    }
  else
    {
      // phares 3/17/98
      //
      // Friction will have been adjusted by friction thinkers for
      // icy or muddy floors. Otherwise it was never touched and
      // remained set at ORIG_FRICTION
      //
      // killough 8/28/98: removed inefficient thinker algorithm,
      // instead using touching_sectorlist in P_GetFriction() to
      // determine friction (and thus only when it is needed).
      //
      // killough 10/98: changed to work with new bobbing method.
      // Reducing player momentum is no longer needed to reduce
      // bobbing, so ice works much better now.

      fixed_t friction = P_GetFriction(mo, NULL);

      mo->momx = FixedMul(mo->momx, friction);
      mo->momy = FixedMul(mo->momy, friction);

      // killough 10/98: Always decrease player bobbing by ORIG_FRICTION.
      // This prevents problems with bobbing on ice, where it was not being
      // reduced fast enough, leading to all sorts of kludges being developed.

      if (player && player->mo == mo)     //  Not voodoo dolls
	{
	  player->momx = FixedMul(player->momx, ORIG_FRICTION);
	  player->momy = FixedMul(player->momy, ORIG_FRICTION);
	}
    }
}

//
// P_ZMovement
//
// Attempt vertical movement.

static void P_ZMovement (mobj_t* mo)
{
  // killough 7/11/98:
  // BFG fireballs bounced on floors and ceilings in Pre-Beta Doom
  // killough 8/9/98: added support for non-missile objects bouncing
  // (e.g. grenade, mine, pipebomb)

  if (mo->flags & MF_BOUNCES && mo->momz)
    {
      mo->z += mo->momz;
      if (mo->z <= mo->floorz)                  // bounce off floors
	{
	  mo->z = mo->floorz;
	  if (mo->momz < 0)
	    {
	      mo->momz = -mo->momz;
	      if (!(mo->flags & MF_NOGRAVITY))  // bounce back with decay
		{
		  mo->momz = mo->flags & MF_FLOAT ?   // floaters fall slowly
		    mo->flags & MF_DROPOFF ?          // DROPOFF indicates rate
		    FixedMul(mo->momz, (fixed_t)(FRACUNIT*.85)) :
		    FixedMul(mo->momz, (fixed_t)(FRACUNIT*.70)) :
		    FixedMul(mo->momz, (fixed_t)(FRACUNIT*.45)) ;
		  
		  // Bring it to rest below a certain speed
                  if (abs(mo->momz) <= mo->info->mass*(gravity*4/256))
		    mo->momz = 0;
		}

	      // killough 11/98: touchy objects explode on impact
	      if (mo->flags & MF_TOUCHY && mo->intflags & MIF_ARMED &&
		  mo->health > 0)
		P_DamageMobj(mo, NULL, NULL, mo->health);
	      else
		if (mo->flags & MF_FLOAT && sentient(mo))
		  goto floater;
	      return;
	    }
	}
      else
	if (mo->z >= mo->ceilingz - mo->height)   // bounce off ceilings
	  {
	    mo->z = mo->ceilingz - mo->height;
	    if (mo->momz > 0)
	      {
		if (mo->subsector->sector->ceilingpic != skyflatnum)
		  mo->momz = -mo->momz;    // always bounce off non-sky ceiling
		else
		  if (mo->flags & MF_MISSILE)
		    P_RemoveMobj(mo);      // missiles don't bounce off skies
		  else
		    if (mo->flags & MF_NOGRAVITY)
		      mo->momz = -mo->momz; // bounce unless under gravity

		if (mo->flags & MF_FLOAT && sentient(mo))
		  goto floater;

		return;
	      }
	  }
	else
	  {
	    if (!(mo->flags & MF_NOGRAVITY))      // free-fall under gravity
              mo->momz -= mo->info->mass*(gravity/256);
	    if (mo->flags & MF_FLOAT && sentient(mo))
	      goto floater;
	    return;
	  }

      // came to a stop
      mo->momz = 0;

      if (mo->flags & MF_MISSILE)
	{
	  if (ceilingline &&
	      ceilingline->backsector &&
	      ceilingline->backsector->ceilingpic == skyflatnum &&
	      mo->z > ceilingline->backsector->ceilingheight)
	    P_RemoveMobj(mo);  // don't explode on skies
	  else
	    P_ExplodeMissile(mo);
	}

      if (mo->flags & MF_FLOAT && sentient(mo))
	goto floater;
      return;
    }

  // killough 8/9/98: end bouncing object code

  // check for smooth step up

  if (mo->player &&
      mo->player->mo == mo &&  // killough 5/12/98: exclude voodoo dolls
      mo->z < mo->floorz)
    {
      mo->player->viewheight -= mo->floorz-mo->z;
      mo->player->deltaviewheight = (VIEWHEIGHT - mo->player->viewheight)>>3;
    }

  // adjust altitude
  mo->z += mo->momz;

floater:

    // float down towards target if too close

    if (!((mo->flags ^ MF_FLOAT) & (MF_FLOAT | MF_SKULLFLY | MF_INFLOAT)) &&
	mo->target)     // killough 11/98: simplify
      {
	fixed_t delta;
	if (P_AproxDistance(mo->x - mo->target->x, mo->y - mo->target->y) <
	    abs(delta = mo->target->z + (mo->height>>1) - mo->z)*3)
	  mo->z += delta < 0 ? -FLOATSPEED : FLOATSPEED;
      }

  // clip movement

  if (mo->z <= mo->floorz)
    {
      // hit the floor

      if (mo->flags & MF_SKULLFLY)
	mo->momz = -mo->momz; // the skull slammed into something

      if (mo->momz < 0)
	{
	  // killough 11/98: touchy objects explode on impact
	  if (mo->flags & MF_TOUCHY && mo->intflags & MIF_ARMED &&
	      mo->health > 0)
	    P_DamageMobj(mo, NULL, NULL, mo->health);
	  else
	    if (mo->player && // killough 5/12/98: exclude voodoo dolls
		mo->player->mo == mo &&
                mo->momz < -gravity*8)
	      {
		// Squat down.
		// Decrease viewheight for a moment
		// after hitting the ground (hard),
		// and utter appropriate sound.

		mo->player->deltaviewheight = mo->momz>>3;
		if(!predicted_tic) // sf: dont play sound in predicted tic
		  S_StartSound (mo, sfx_oof);
	      }
	  mo->momz = 0;
	}

      mo->z = mo->floorz;

      if (!((mo->flags ^ MF_MISSILE) & (MF_MISSILE | MF_NOCLIP)))
	{
	  P_ExplodeMissile (mo);
	  return;
	}
    }
  else // still above the floor
    if (!(mo->flags & MF_NOGRAVITY))
      {
	if (!mo->momz)
          mo->momz = -gravity;
        mo->momz -= gravity;
      }

  if (mo->z + mo->height > mo->ceilingz)
    {
      // hit the ceiling

      if (mo->momz > 0)
	mo->momz = 0;

      mo->z = mo->ceilingz - mo->height;

      if (mo->flags & MF_SKULLFLY)
	mo->momz = -mo->momz; // the skull slammed into something

      if (!((mo->flags ^ MF_MISSILE) & (MF_MISSILE | MF_NOCLIP)))
	{
	  P_ExplodeMissile (mo);
	  return;
	}
    }
}

//
// P_NightmareRespawn
//

void P_NightmareRespawn(mobj_t* mobj)
{
  fixed_t      x;
  fixed_t      y;
  fixed_t      z;
  subsector_t* ss;
  mobj_t*      mo;
  mapthing_t*  mthing;

  x = mobj->spawnpoint.x << FRACBITS;
  y = mobj->spawnpoint.y << FRACBITS;

  // something is occupying its position?

  if (!P_CheckPosition (mobj, x, y))
    return; // no respwan

  // spawn a teleport fog at old spot
  // because of removal of the body?

  mo = P_SpawnMobj(mobj->x, mobj->y,
		   mobj->subsector->sector->floorheight, MT_TFOG);

  // initiate teleport sound

  S_StartSound (mo, sfx_telept);

  // spawn a teleport fog at the new spot

  ss = R_PointInSubsector (x,y);

  mo = P_SpawnMobj (x, y, ss->sector->floorheight , MT_TFOG);

  S_StartSound (mo, sfx_telept);

  // spawn the new monster

  mthing = &mobj->spawnpoint;
  z = mobj->info->flags & MF_SPAWNCEILING ? ONCEILINGZ : ONFLOORZ;

  // inherit attributes from deceased one

  mo = P_SpawnMobj (x,y,z, mobj->type);
  mo->spawnpoint = mobj->spawnpoint;
                // sf: use R_WadToAngle
  mo->angle = R_WadToAngle(mthing->angle);

  if (mthing->options & MTF_AMBUSH)
    mo->flags |= MF_AMBUSH;

  // killough 11/98: transfer friendliness from deceased
  mo->flags = (mo->flags & ~MF_FRIEND) | (mobj->flags & MF_FRIEND);

  mo->reactiontime = 18;

  // remove the old monster,

  P_RemoveMobj (mobj);
}

//
// P_MobjThinker
//

void P_MobjThinker (mobj_t* mobj)
{
  // killough 11/98: 
  // removed old code which looked at target references
  // (we use pointer reference counting now)

  // momentum movement
  if (mobj->momx | mobj->momy || mobj->flags & MF_SKULLFLY)
    {
      P_XYMovement(mobj);
      if (mobj->thinker.function == P_RemoveThinkerDelayed) // killough
	return;       // mobj was removed
    }

  if (mobj->z != mobj->floorz || mobj->momz)
    {
      P_ZMovement(mobj);
      if (mobj->thinker.function == P_RemoveThinkerDelayed) // killough
	return;       // mobj was removed
    }
  else
    if (!(mobj->momx | mobj->momy) && !sentient(mobj))
      {                                  // non-sentient objects at rest
	mobj->intflags |= MIF_ARMED;     // arm a mine which has come to rest

	// killough 9/12/98: objects fall off ledges if they are hanging off
	// slightly push off of ledge if hanging more than halfway off

	if (mobj->z > mobj->dropoffz &&      // Only objects contacting dropoff
	    !(mobj->flags & MF_NOGRAVITY) && // Only objects which fall
	    !comp[comp_falloff] && demo_version >= 203) // Not in old demos
	  P_ApplyTorque(mobj);               // Apply torque
	else
	  mobj->intflags &= ~MIF_FALLING, mobj->gear = 0;  // Reset torque
      }

  // cycle through states,
  // calling action functions at transitions
  // killough 11/98: simplify

  if (mobj->tics != -1)      // you can cycle through multiple states in a tic
    {
      if (!--mobj->tics)
	P_SetMobjState(mobj, mobj->state->nextstate);
    }
  else                       
    if (mobj->flags & MF_COUNTKILL && respawnmonsters &&
	++mobj->movecount >= 12*35 && !(leveltime & 31) &&
	P_Random (pr_respawn) <= 4)
      P_NightmareRespawn(mobj);          // check for nightmare respawn
}


//
// P_SpawnMobj
//

mobj_t *P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type)
{
  mobj_t *mobj = Z_Malloc(sizeof *mobj, PU_LEVEL, NULL);
  mobjinfo_t *info = &mobjinfo[type];
  state_t    *st;

  memset(mobj, 0, sizeof *mobj);

  mobj->type = type;
  mobj->info = info;
  mobj->x = x;
  mobj->y = y;
  mobj->radius = info->radius;
  mobj->height = info->height;                                      // phares
  mobj->flags  = info->flags;
  mobj->skin = NULL;

  // killough 8/23/98: no friends, bouncers, or touchy things in old demos
  if (demo_version < 203)
    mobj->flags &= ~(MF_BOUNCES | MF_FRIEND | MF_TOUCHY); 
  else          // sf: not friends in deathmatch!
                // extra version check for old (mbf) demos
    if (demo_version < 303 || !deathmatch)
      if (type == MT_PLAYER)         // Except in old demos, players
        mobj->flags |= MF_FRIEND;    // are always friends.

  mobj->health = info->spawnhealth;

  if (gameskill != sk_nightmare)
    mobj->reactiontime = info->reactiontime;

  mobj->lastlook = P_Random (pr_lastlook) % MAXPLAYERS;

  // do not set the state with P_SetMobjState,
  // because action routines can not be called yet

  st = &states[info->spawnstate];

  mobj->state  = st;
  mobj->tics   = st->tics;
  mobj->sprite = st->sprite;
  mobj->frame  = st->frame;

  // NULL head of sector list // phares 3/13/98
  mobj->touching_sectorlist = NULL;

  // set subsector and/or block links

  P_SetThingPosition(mobj);

  mobj->dropoffz =           // killough 11/98: for tracking dropoffs
  mobj->floorz   = mobj->subsector->sector->floorheight;
  mobj->ceilingz = mobj->subsector->sector->ceilingheight;
  
  mobj->z = z == ONFLOORZ ? mobj->floorz : z == ONCEILINGZ ?
    mobj->ceilingz - mobj->height : z;

  mobj->thinker.function = P_MobjThinker;
  mobj->above_thing = mobj->below_thing = 0;           // phares

  P_AddThinker(&mobj->thinker);

  mobj->colour = (info->flags & MF_TRANSLATION) >> MF_TRANSSHIFT;

  // sf: move kills/items check here, for FraggleScript
  // ignore friends
  if(mobj->flags & MF_COUNTKILL && !(mobj->flags & MF_FRIEND))
      totalkills++;
      
  if(mobj->flags & MF_COUNTITEM)
    totalitems++;
    
  return mobj;
}

static mapthing_t itemrespawnque[ITEMQUESIZE];
static int itemrespawntime[ITEMQUESIZE];
int iquehead, iquetail;
int respawnsuper=0;

//
// P_RemoveMobj
//

void P_RemoveMobj (mobj_t *mobj)
{
  if (!((mobj->flags ^ MF_SPECIAL) & (MF_SPECIAL | MF_DROPPED))
		|| mobj->type==MT_BARREL)
    {
      if((mobj->type == MT_INV || mobj->type == MT_INS) && !respawnsuper)
	{ // sf: respawning super-powerups
	}
      else
	{
	  if(mobj->type==MT_BARREL && deathmatch!=3)
	    {       // deathmatch 3: barrels respawn
	    }
	  else
	    {
	      itemrespawnque[iquehead] = mobj->spawnpoint;
	      itemrespawntime[iquehead++] = leveltime;
	      if ((iquehead &= ITEMQUESIZE-1) == iquetail)
		// lose one off the end?
		iquetail = (iquetail+1)&(ITEMQUESIZE-1);
	    }
	}
    }
  
  // unlink from sector and block lists

  P_UnsetThingPosition (mobj);

  // Delete all nodes on the current sector_list               phares 3/16/98
  if (sector_list)
    {
      P_DelSeclist(sector_list);
      sector_list = NULL;
    }

  // stop any playing sound

  S_StopSound (mobj);

  // killough 11/98:
  //
  // Remove any references to other mobjs.
  //
  // Older demos might depend on the fields being left alone, however,
  // if multiple thinkers reference each other indirectly before the
  // end of the current tic.

  if (demo_version >= 203)
    {
      P_SetTarget(&mobj->target,    NULL);
      P_SetTarget(&mobj->tracer,    NULL);
      P_SetTarget(&mobj->lastenemy, NULL);
    }

  // free block

  P_RemoveThinker(&mobj->thinker);
}

//
// P_FindDoomedNum
//
// Finds a mobj type with a matching doomednum
//
// killough 8/24/98: rewrote to use hashing
//

// sf: fix thing hash for dynamic wad loading

static struct
{ int first, next; }
*thing_hash;

static void P_BuildThingHash()
{
  int i;
  
  if(thing_hash)
    Z_Free(thing_hash);

  thing_hash = Z_Malloc(sizeof *thing_hash * NUMMOBJTYPES,
			PU_CACHE, (void **) &thing_hash);

  for (i=0; i<NUMMOBJTYPES; i++)
    thing_hash[i].first = NUMMOBJTYPES;
  for (i=0; i<NUMMOBJTYPES; i++)
    if (mobjinfo[i].doomednum != -1)
      {
	unsigned h = (unsigned) mobjinfo[i].doomednum % NUMMOBJTYPES;
	thing_hash[i].next = thing_hash[h].first;
	thing_hash[h].first = i;
      }
}

void P_ClearThingHash()
{
  if(thing_hash)
    {
      Z_Free(thing_hash);
      thing_hash = NULL;
    }
}

int P_FindDoomedNum(unsigned type)
{
  register int i;

  if (!thing_hash)
    P_BuildThingHash();
    
  i = thing_hash[type % NUMMOBJTYPES].first;
  while (i < NUMMOBJTYPES && mobjinfo[i].doomednum != type)
    i = thing_hash[i].next;
  return i;
}

//
// P_RespawnSpecials
//

void P_RespawnSpecials (void)
{
  fixed_t x, y, z;
  subsector_t*  ss;
  mobj_t*       mo;
  mapthing_t*   mthing;
  int           i;

  if (deathmatch < 2 ||       // only respawn items in deathmatch
      iquehead == iquetail ||  // nothing left to respawn?
      leveltime - itemrespawntime[iquetail] < 30*35) // wait 30 seconds
    return;
  
  mthing = &itemrespawnque[iquetail];

  x = mthing->x << FRACBITS;
  y = mthing->y << FRACBITS;

  // spawn a teleport fog at the new spot

  ss = R_PointInSubsector (x,y);
  mo = P_SpawnMobj(x, y, ss->sector->floorheight , MT_IFOG);
  S_StartSound(mo, sfx_itmbk);

  // find which type to spawn

  // killough 8/23/98: use table for faster lookup
  i = P_FindDoomedNum(mthing->type);

  // spawn it
  z = mobjinfo[i].flags & MF_SPAWNCEILING ? ONCEILINGZ : ONFLOORZ;

  mo = P_SpawnMobj(x,y,z, i);
  mo->spawnpoint = *mthing;
                // sf
  mo->angle = R_WadToAngle(mthing->angle);

  // pull it from the queue

  iquetail = (iquetail+1)&(ITEMQUESIZE-1);
}

//
// P_SpawnPlayer
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged
//  between levels.
//

void P_SpawnPlayer (mapthing_t* mthing)
{
  player_t* p;
  fixed_t   x, y, z;
  mobj_t*   mobj;
  int       i;

  // not playing?

  if (!playeringame[mthing->type-1])
    return;

  p = &players[mthing->type-1];

  if (p->playerstate == PST_REBORN)
    G_PlayerReborn (mthing->type-1);

  x    = mthing->x << FRACBITS;
  y    = mthing->y << FRACBITS;
  z    = ONFLOORZ;
  mobj = P_SpawnMobj (x,y,z, MT_PLAYER);

  // set color translations for player sprites

//  if (mthing->type > 1)
  mobj->colour = players[mthing->type-1].colormap; //sf

  mobj->angle      = R_WadToAngle(mthing->angle);
  mobj->player     = p;
  mobj->health     = p->health;
  mobj->skin       = p->skin;
  mobj->sprite     = p->skin->sprite;

  p->mo            = mobj;
  p->playerstate   = PST_LIVE;
  p->refire        = 0;
  p->damagecount   = 0;
  p->bonuscount    = 0;
  p->extralight    = 0;
  p->fixedcolormap = 0;
  p->viewheight    = VIEWHEIGHT;
  p->viewz         = mobj->z + VIEWHEIGHT;

  p->momx = p->momy = 0;   // killough 10/98: initialize bobbing to 0.

  // setup gun psprite

  P_SetupPsprites (p);

  // give all cards in death match mode

  if (deathmatch)
    for (i = 0 ; i < NUMCARDS ; i++)
      p->cards[i] = true;

  if (mthing->type-1 == consoleplayer)
    {
      ST_Start(); // wake up the status bar
      HU_Start(); // wake up the heads up text
    }
  if(mthing->type-1 == displayplayer)
    P_ResetChasecam(); //sf
}


//
// P_SpawnMapThing
// The fields of the mapthing should
// already be in host byte order.
//
// sf: made to return mobj_t* spawned

mobj_t *P_SpawnMapThing (mapthing_t* mthing)
{
  int    i;
  mobj_t *mobj;
  fixed_t x, y, z;

  switch(mthing->type)
    {
    case 0:             // killough 2/26/98: Ignore type-0 things as NOPs
    case DEN_PLAYER5:   // phares 5/14/98: Ignore Player 5-8 starts (for now)
    case DEN_PLAYER6:
    case DEN_PLAYER7:
    case DEN_PLAYER8:
      return NULL;
    }

  // killough 11/98: clear flags unused by Doom
  //
  // We clear the flags unused in Doom if we see flag mask 256 set, since
  // it is reserved to be 0 under the new scheme. A 1 in this reserved bit
  // indicates it's a Doom wad made by a Doom editor which puts 1's in
  // bits that weren't used in Doom (such as HellMaker wads). So we should
  // then simply ignore all upper bits.

  if (demo_compatibility || 
      (demo_version >= 203 && mthing->options & MTF_RESERVED))
    mthing->options &= MTF_EASY|MTF_NORMAL|MTF_HARD|MTF_AMBUSH|MTF_NOTSINGLE;

  // count deathmatch start positions

  if (mthing->type == 11)
    {
      // 1/11/98 killough -- new code removes limit on deathmatch starts:

      size_t offset = deathmatch_p - deathmatchstarts;

      if (offset >= num_deathmatchstarts)
	{
	  num_deathmatchstarts = num_deathmatchstarts ?
	    num_deathmatchstarts*2 : 16;
	  deathmatchstarts = realloc(deathmatchstarts,
				     num_deathmatchstarts *
				     sizeof(*deathmatchstarts));
	  deathmatch_p = deathmatchstarts + offset;
	}
      memcpy(deathmatch_p++, mthing, sizeof*mthing);
      return NULL; //sf
    }

  if(mthing->type == 5003)
    {
      // save for intermissions
      WI_AddCamera(mthing);
      return NULL;
    }

  // check for players specially

  if (mthing->type <= 4 && mthing->type > 0) // killough 2/26/98 -- fix crashes
    {
#ifdef DOGS
      // killough 7/19/98: Marine's best friend :)
      if (!netgame && mthing->type > 1 && mthing->type <= dogs+1 &&
	  !players[mthing->type-1].secretcount)
	{  // use secretcount to avoid multiple dogs in case of multiple starts
	  players[mthing->type-1].secretcount = 1;

	  // killough 10/98: force it to be a friend
	  mthing->options |= MTF_FRIEND;
	  i = MT_DOGS;
	  goto spawnit;
	}
#endif

      // save spots for respawning in network games
      playerstarts[mthing->type-1] = *mthing;
      if (!deathmatch)
	P_SpawnPlayer (mthing);

      return NULL; //sf
    }

  // check for apropriate skill level

  if (!netgame && mthing->options & MTF_NOTSINGLE)//jff "not single" thing flag
    return NULL; //sf

  //jff 3/30/98 implement "not deathmatch" thing flag

  if (netgame && deathmatch && mthing->options & MTF_NOTDM)
    return NULL; //sf

  //jff 3/30/98 implement "not cooperative" thing flag

  if (netgame && !deathmatch && mthing->options & MTF_NOTCOOP)
    return NULL;  // sf

  // killough 11/98: simplify
  if (gameskill == sk_baby || gameskill == sk_easy ? 
      !(mthing->options & MTF_EASY) :
      gameskill == sk_hard || gameskill == sk_nightmare ?
      !(mthing->options & MTF_HARD) : !(mthing->options & MTF_NORMAL))
    return NULL;  // sf

  // find which type to spawn

  // killough 8/23/98: use table for faster lookup
  i = P_FindDoomedNum(mthing->type);

  // phares 5/16/98:
  // Do not abort because of an unknown thing. Ignore it, but post a
  // warning message for the player.

  if (i == NUMMOBJTYPES)
    {
      doom_printf("Unknown Thing type %i at (%i, %i)",
	      mthing->type, mthing->x, mthing->y);
      return NULL;  // sf
    }

  // don't spawn keycards and players in deathmatch

  if (deathmatch && mobjinfo[i].flags & MF_NOTDMATCH)
    return NULL;        // sf

  // don't spawn any monsters if -nomonsters

  if (nomonsters && (i == MT_SKULL || (mobjinfo[i].flags & MF_COUNTKILL)))
    return NULL;        // sf

#ifdef DOGS
  // spawn it
spawnit:
#endif

  x = mthing->x << FRACBITS;
  y = mthing->y << FRACBITS;

  z = mobjinfo[i].flags & MF_SPAWNCEILING ? ONCEILINGZ : ONFLOORZ;

  mobj = P_SpawnMobj (x,y,z, i);
  mobj->spawnpoint = *mthing;

  if (mobj->tics > 0)
    mobj->tics = 1 + (P_Random (pr_spawnthing) % mobj->tics);

  if (!(mobj->flags & MF_FRIEND) &&
      mthing->options & MTF_FRIEND && 
      demo_version>=203)
    {
      mobj->flags |= MF_FRIEND;            // killough 10/98:
      P_UpdateThinker(&mobj->thinker);     // transfer friendliness flag
    }

  mobj->angle = R_WadToAngle(mthing->angle);
  if (mthing->options & MTF_AMBUSH)
    mobj->flags |= MF_AMBUSH;

  return mobj;
}

//
// GAME SPAWN FUNCTIONS
//

//
// P_SpawnPuff
//

extern fixed_t attackrange;

void P_SpawnPuff(fixed_t x,fixed_t y,fixed_t z)
{
  mobj_t* th;
  // killough 5/5/98: remove dependence on order of evaluation:
  int t = P_Random(pr_spawnpuff);
  z += (t - P_Random(pr_spawnpuff))<<10;

  th = P_SpawnMobj (x,y,z, MT_PUFF);
  th->momz = FRACUNIT;
  th->tics -= P_Random(pr_spawnpuff)&3;

  if (th->tics < 1)
    th->tics = 1;

  // don't make punches spark on the wall

  if (attackrange == MELEERANGE)
    P_SetMobjState (th, S_PUFF3);
}


//
// P_SpawnBlood
//
void P_SpawnBlood(fixed_t x,fixed_t y,fixed_t z,int damage)
{
  mobj_t* th;
  // killough 5/5/98: remove dependence on order of evaluation:
  int t = P_Random(pr_spawnblood);
  z += (t - P_Random(pr_spawnblood))<<10;
  th = P_SpawnMobj(x,y,z, MT_BLOOD);
  th->momz = FRACUNIT*2;
  th->tics -= P_Random(pr_spawnblood)&3;

  if (th->tics < 1)
    th->tics = 1;

  if (damage <= 12 && damage >= 9)
    P_SetMobjState(th,S_BLOOD2);
  else
    if (damage < 9)
      P_SetMobjState(th,S_BLOOD3);
}

void P_SpawnParticle(fixed_t x, fixed_t y, fixed_t z)
{
  P_SpawnMobj(x, y, z, MT_PARTICLE);
}


void P_ParticleLine(mobj_t *source, mobj_t *dest)
{
  fixed_t sourcex, sourcey, sourcez;
  fixed_t destx, desty, destz;
  int linedetail;
  int j;
  
  sourcex = source->x; sourcey = source->y;
  destx = dest->x; desty = dest->y;
  sourcez = source->z + (source->info->height/2);
  destz = dest->z + (dest->info->height/2);
  linedetail = P_AproxDistance(destx - sourcex, desty - sourcey)
                                / FRACUNIT;
  
  // make the line
  for(j=0; j<linedetail; j++)
    P_SpawnParticle(
		    sourcex + ((destx - source->x)*j)/linedetail,
		    sourcey + ((desty - source->y)*j)/linedetail,
		    sourcez + ((destz - source->z)*j)/linedetail);
}

//
// P_CheckMissileSpawn
// Moves the missile forward a bit
//  and possibly explodes it right there.
//

void P_CheckMissileSpawn (mobj_t* th)
{
  th->tics -= P_Random(pr_missile)&3;
  if (th->tics < 1)
    th->tics = 1;

  // move a little forward so an angle can
  // be computed if it immediately explodes

  th->x += th->momx>>1;
  th->y += th->momy>>1;
  th->z += th->momz>>1;

  // killough 8/12/98: for non-missile objects (e.g. grenades)
  if (!(th->flags & MF_MISSILE) && demo_version >= 203)
    return;

  // killough 3/15/98: no dropoff (really = don't care for missiles)
  if (!P_TryMove(th, th->x, th->y, false))
    P_ExplodeMissile (th);
}

//
// P_SpawnMissile
//

mobj_t* P_SpawnMissile(mobj_t* source,mobj_t* dest,mobjtype_t type)
{
  angle_t an;
  int     dist;
  mobj_t *th = P_SpawnMobj (source->x,source->y,source->z + 4*8*FRACUNIT,type);

  if (th->info->seesound)
    S_StartSound (th, th->info->seesound);

  P_SetTarget(&th->target, source);    // where it came from // killough 11/98
  an = R_PointToAngle2 (source->x, source->y, dest->x, dest->y);

  // fuzzy player
  if (dest->flags & MF_SHADOW)
    {  // killough 5/5/98: remove dependence on order of evaluation:
      int t = P_Random(pr_shadow);
      an += (t - P_Random(pr_shadow))<<20;
    }

  th->angle = an;
  an >>= ANGLETOFINESHIFT;
  th->momx = FixedMul (th->info->speed, finecosine[an]);
  th->momy = FixedMul (th->info->speed, finesine[an]);

  dist = P_AproxDistance (dest->x - source->x, dest->y - source->y);
  dist = dist / th->info->speed;

  if (dist < 1)
    dist = 1;

  th->momz = (dest->z - source->z) / dist;
  P_CheckMissileSpawn(th);

  return th;
}

//
// P_SpawnPlayerMissile
// Tries to aim at a nearby monster
//
extern fixed_t *yslope;

mobj_t *P_SpawnPlayerMissile(mobj_t* source, mobjtype_t type)
{
  mobj_t *th;
  fixed_t x, y, z, slope = 0;

  // see which target is to be aimed at

  angle_t an = source->angle;

  // killough 7/19/98: autoaiming was not in original beta
        // sf: made a multiplayer option
  if (autoaim)
    {
      // killough 8/2/98: prefer autoaiming at enemies
      int mask = demo_version < 203 ? 0 : MF_FRIEND;
      do
	{
	  slope = P_AimLineAttack(source, an, 16*64*FRACUNIT, mask);
	  if (!linetarget)
	    slope = P_AimLineAttack(source, an += 1<<26, 16*64*FRACUNIT, mask);
	  if (!linetarget)
	    slope = P_AimLineAttack(source, an -= 2<<26, 16*64*FRACUNIT, mask);
	  if (!linetarget)
	  {
	    an = source->angle;
            if(source->player->readyweapon!=wp_bfg || bfglook==1)
              slope = source->player->updownangle * LOOKSLOPE;
	    else
	      slope = 0;
	  }
	}
      while (mask && (mask=0, !linetarget));  // killough 8/2/98
    }
    else
        slope = source->player->updownangle * LOOKSLOPE;

  x = source->x;
  y = source->y;
  z = source->z + 4*8*FRACUNIT;

  th = P_SpawnMobj (x,y,z, type);

  if (th->info->seesound)
    S_StartSound (th, th->info->seesound);

  P_SetTarget(&th->target, source);   // killough 11/98
  th->angle = an;
  th->momx = FixedMul(th->info->speed,finecosine[an>>ANGLETOFINESHIFT]);
  th->momy = FixedMul(th->info->speed,finesine[an>>ANGLETOFINESHIFT]);
  th->momz = FixedMul(th->info->speed,slope);

  P_CheckMissileSpawn(th);

  return th;    //sf
}

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.3  2000-05-03 16:21:23  fraggle
// client speedup code
//
// Revision 1.2  2000/05/02 15:43:41  fraggle
// client movement prediction
//
// Revision 1.1.1.1  2000/04/30 19:12:08  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
