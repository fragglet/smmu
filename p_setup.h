// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_setup.h,v 1.3 1998/05/03 23:03:31 killough Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//   Setup a game, startup stuff.
//
//-----------------------------------------------------------------------------

#ifndef __P_SETUP__
#define __P_SETUP__

#include "p_mobj.h"

void P_SetupLevel(char*, int playermask, skill_t skill);
void P_Init(void);               // Called by startup code.

extern byte     *rejectmatrix;   // for fast sight rejection

// killough 3/1/98: change blockmap from "short" to "long" offsets:
extern long     *blockmaplump;   // offsets in blockmap are from here
extern long     *blockmap;
extern int      bmapwidth;
extern int      bmapheight;      // in mapblocks
extern fixed_t  bmaporgx;
extern fixed_t  bmaporgy;        // origin of block map
extern mobj_t   **blocklinks;    // for thing chains

extern int      newlevel;
extern int      doom1level;
extern char     levelmapname[10];

typedef struct                          // Standard OLO stuff, put in WADs
{       
        unsigned char header[3];                 // Header
        unsigned char space1;
        unsigned char extend;
        unsigned char space2;
                                        // Standard
        unsigned char levelwarp;
        unsigned char lastlevel;
        unsigned char deathmatch;
        unsigned char skill_level;
        unsigned char nomonsters;
        unsigned char respawn;
        unsigned char fast;

        unsigned char levelname[32][32];
} olo_t;

extern olo_t olo;
extern int olo_loaded;

#endif

//----------------------------------------------------------------------------
//
// $Log: p_setup.h,v $
// Revision 1.3  1998/05/03  23:03:31  killough
// beautification, add external declarations for blockmap
//
// Revision 1.2  1998/01/26  19:27:28  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:08  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
