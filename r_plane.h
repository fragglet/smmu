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
//      Refresh, visplane stuff (floor, ceilings).
//
//-----------------------------------------------------------------------------

#ifndef __R_PLANE__
#define __R_PLANE__

#include "r_data.h"

// killough 10/98: special mask indicates sky flat comes from sidedef
#define PL_SKYFLAT (0x80000000)

// Visplane related.
extern  short *lastopening;

extern short floorclip[], ceilingclip[];
extern short floorclip2[], ceilingclip2[]; //sf
extern fixed_t *yslope;
extern fixed_t origyslope[], distscale[];

void R_InitPlanes(void);
void R_ClearPlanes(void);
void R_DrawPlanes (void);

visplane_t *R_FindPlane(
                        fixed_t height, 
                        int picnum,
                        int lightlevel,
                        fixed_t xoffs,  // killough 2/28/98: add x-y offsets
                        fixed_t yoffs );

visplane_t *R_CheckPlane(visplane_t *pl, int start, int stop);

extern int visplane_view;

#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:09  fraggle
// Initial revision
//
//
//----------------------------------------------------------------------------
