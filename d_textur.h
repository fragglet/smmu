// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
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
//    Typedefs related to to textures etc.,
//    isolated here to make it easier separating modules.
//    
//-----------------------------------------------------------------------------


#ifndef __D_TEXTUR__
#define __D_TEXTUR__

#include "doomtype.h"


// NOTE: Checking all BOOM sources, there is nothing used called pic_t.

//
// Flats?
//
// a pic is an unmasked block of pixels
typedef struct
{
  byte  width;
  byte  height;
  byte  data;
} pic_t;


#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-07-29 13:20:39  fraggle
// Initial revision
//
// Revision 1.3  1998/05/04  21:34:18  thldrmn
// commenting and reformatting
//
// Revision 1.2  1998/01/26  19:26:33  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:54  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
