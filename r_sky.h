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
//      Sky rendering.
//
//-----------------------------------------------------------------------------

#ifndef __R_SKY__
#define __R_SKY__

#include "m_fixed.h"

// SKY, store the number for name.
#define SKYFLATNAME  "F_SKY1"

// The sky map is 256*128*4 maps.
#define ANGLETOSKYSHIFT         22

extern int skytexture;
extern int skytexturemid;
extern int stretchsky;

// Called whenever the view size changes.
void R_InitSkyMap(void);

// init sky at start of level
void R_StartSky();

#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:09  fraggle
// Initial revision
//
//
//----------------------------------------------------------------------------
