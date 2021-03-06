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
//      System specific interface stuff.
//
//-----------------------------------------------------------------------------

#ifndef __R_MAIN__
#define __R_MAIN__

#include "d_player.h"
#include "r_data.h"
#include "r_defs.h"
#include "p_chase.h"

//
// POV related.
//

extern fixed_t    viewcos;
extern fixed_t    viewsin;
extern int        viewwidth;
extern int        viewheight;
extern int        viewwindowx;
extern int        viewwindowy;
extern sector_t  *viewsector;
extern mobj_t    *viewobj;        // sf
extern int        centerx;
extern int        centery;
extern fixed_t    centerxfrac;
extern fixed_t    centeryfrac;
extern fixed_t    projection;
extern int        validcount;
extern int        linecount;
extern int        loopcount;

extern boolean    showpsprites;

//
// Lighting LUT.
// Used for z-depth cuing per column/row,
//  and other lighting effects (sector ambient, flash).
//

// Lighting constants.

#define LIGHTLEVELS       16
#define LIGHTSEGSHIFT      4
#define LIGHTSCALESHIFT   12
#define LIGHTZSHIFT       20

// killough 3/20/98: Allow colormaps to be dynamic (e.g. underwater)
extern lighttable_t *(*scalelight)[MAXLIGHTSCALE];
extern lighttable_t *(*zlight)[MAXLIGHTZ];
extern lighttable_t *fullcolormap;
extern int numcolormaps;    // killough 4/4/98: dynamic number of maps
extern lighttable_t **colormaps;
// killough 3/20/98, 4/4/98: end dynamic colormaps

extern int          extralight;
extern lighttable_t *fixedcolormap;

// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.

#define NUMCOLORMAPS 32

//
// Function pointer to switch refresh/drawing functions.
//

extern void (*colfunc)(void);

//
// Utility functions.
//

int R_PointOnSide(fixed_t x, fixed_t y, node_t *node);
int R_PointOnSegSide(fixed_t x, fixed_t y, seg_t *line);
angle_t R_PointToAngle(fixed_t x, fixed_t y);
angle_t R_PointToAngle2(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2);
fixed_t R_ScaleFromGlobalAngle(angle_t visangle);
subsector_t *R_PointInSubsector(fixed_t x, fixed_t y);
void R_SectorColormap(sector_t *s);

//
// REFRESH - the actual rendering functions.
//

                // sf: camera point added
void R_RenderPlayerView(player_t *player, camera_t *viewcamera); // Called by G_Drawer.
                                // sf: G_Drawer???
void R_Init(void);                           // Called by startup code.
void R_SetViewSize(int blocks);              // Called by M_Responder.

void R_InitLightTables(void);                // killough 8/9/98

extern boolean setsizeneeded;
void R_ExecuteSetViewSize(void);

angle_t R_WadToAngle(int wadangle);

extern int flatskip;
extern int viewdir;
extern fixed_t zoomscale;

#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:09  fraggle
// Initial revision
//
//
//----------------------------------------------------------------------------
