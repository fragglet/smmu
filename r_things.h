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
//      Rendering of moving objects, sprites.
//
//-----------------------------------------------------------------------------

#ifndef __R_THINGS__
#define __R_THINGS__

#include "r_defs.h"

// Constant arrays used for psprite clipping and initializing clipping.

extern short negonearray[MAX_SCREENWIDTH];         // killough 2/8/98:
extern short screenheightarray[MAX_SCREENWIDTH];   // change to MAX_*

// Vars for R_DrawMaskedColumn

extern short   *mfloorclip;
extern short   *mceilingclip;
extern fixed_t spryscale;
extern fixed_t sprtopscreen;
extern fixed_t pspritescale;
extern fixed_t pspriteiscale;

void R_DrawMaskedColumn(column_t *column);
void R_SortVisSprites(void);
void R_AddSprites(sector_t *sec,int); // killough 9/18/98
void R_AddPSprites(void);
void R_DrawSprites(void);
void R_InitSprites(char **namelist);
void R_ClearSprites(void);
void R_DrawMasked(void);

void R_ClipVisSprite(vissprite_t *vis, int xl, int xh);

#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:09  fraggle
// Initial revision
//
//
//----------------------------------------------------------------------------
