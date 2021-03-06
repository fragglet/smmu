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
//      Status bar code.
//      Does the face/direction indicator animatin.
//      Does palette indicators as well (red pain/berserk, bright pickup)
//
//-----------------------------------------------------------------------------

#ifndef __STSTUFF_H__
#define __STSTUFF_H__

#include "doomtype.h"
#include "d_event.h"
#include "r_defs.h"

// Size of statusbar.
// Now sensitive for scaling.

#define ST_HEIGHT (32*SCREEN_MUL)
#define ST_WIDTH  SCREENWIDTH
#define ST_Y      (SCREENHEIGHT - ST_HEIGHT)

//
// STATUS BAR
//

// Called by main loop.
boolean ST_Responder(event_t* ev);

// Called by main loop.
void ST_Ticker(void);

// Called by main loop.
void ST_Drawer(boolean fullscreen, boolean refresh);

// Called when the console player is spawned on each level.
void ST_Start(void);

// Called by startup code.
void ST_Init(void);

void ST_CacheFaces(patch_t **faces, char *facename);

// for wad loading: clear out graphics and reload
void ST_reloadData();

// States for status bar code.
typedef enum
{
  AutomapState,
  FirstPersonState
} st_stateenum_t;

boolean ST_Responder(event_t* ev);

// killough 5/2/98: moved from m_misc.c:

extern int health_red;    // health amount less than which status is red
extern int health_yellow; // health amount less than which status is yellow
extern int health_green;  // health amount above is blue, below is green
extern int armor_red;     // armor amount less than which status is red
extern int armor_yellow;  // armor amount less than which status is yellow
extern int armor_green;   // armor amount above is blue, below is green
extern int ammo_red;      // ammo percent less than which status is red
extern int ammo_yellow;   // ammo percent less is yellow more green
extern int sts_always_red;// status numbers do not change colors
extern int sts_pct_always_gray;// status percents do not change colors
extern int sts_traditional_keys;  // display keys the traditional way

// Number of status faces.
#define ST_NUMPAINFACES         5
#define ST_NUMSTRAIGHTFACES     3
#define ST_NUMTURNFACES         2
#define ST_NUMSPECIALFACES      3

#define ST_FACESTRIDE \
          (ST_NUMSTRAIGHTFACES+ST_NUMTURNFACES+ST_NUMSPECIALFACES)

#define ST_NUMEXTRAFACES        2

#define ST_NUMFACES \
          (ST_FACESTRIDE*ST_NUMPAINFACES+ST_NUMEXTRAFACES)

extern patch_t *default_faces[ST_NUMFACES];

#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:09  fraggle
// Initial revision
//
//
//----------------------------------------------------------------------------
