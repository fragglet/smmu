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

#ifndef __P_TICK__
#define __P_TICK__

#include "d_think.h"

// Called by C_Ticker, can call G_PlayerExited.
// Carries out all thinking of monsters and players.

void P_Ticker(void);

extern thinker_t thinkercap;  // Both the head and tail of the thinker list

void P_InitThinkers(void);
void P_AddThinker(thinker_t *thinker);
void P_RemoveThinker(thinker_t *thinker);
void P_RemoveThinkerDelayed(thinker_t *thinker);    // killough 4/25/98

void P_UpdateThinker(thinker_t *thinker);   // killough 8/29/98

void P_SetTarget(mobj_t **mo, mobj_t *target);   // killough 11/98

// killough 8/29/98: threads of thinkers, for more efficient searches
typedef enum {
  th_misc,
  th_friends,
  th_enemies,
  NUMTHCLASS
} th_class;

extern thinker_t thinkerclasscap[];

        // sf: jumping-viewz-on-hyperlift bug
extern boolean reset_viewz;
#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:09  fraggle
// Initial revision
//
//
//----------------------------------------------------------------------------
