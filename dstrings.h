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
//
// DESCRIPTION:
//	DOOM strings, by language.
//
//-----------------------------------------------------------------------------


#ifndef __DSTRINGS__
#define __DSTRINGS__


// All important printed strings.
// Language selection (message strings).
// Use -DFRENCH etc.

#ifdef FRENCH
#include "d_french.h"
#else
#include "d_englsh.h"
#endif

// Misc. other strings.
#define SAVEGAMENAME	"doomsav"


//
// File locations,
//  relative to current position.
// Path names are OS-sensitive.
//
#define DEVMAPS "devmaps"
#define DEVDATA "devdata"


// Not done in french?

// QuitDOOM messages
#define NUM_QUITMESSAGES   22

extern char* endmsg[];


#ifndef PD_BLUEC        // some files don't have boom-specific things

#define PD_BLUEC  PD_BLUEK
#define PD_REDC   PD_REDK
#define PD_YELLOWC  PD_YELLOWK
#define PD_BLUES    "You need a blue skull to open this door"
#define PD_REDS     "You need a red skull to open this door"
#define PD_YELLOWS  "You need a yellow skull to open this door"
#define PD_ANY      "Any key will open this door"
#define PD_ALL3     "You need all three keys to open this door"
#define PD_ALL6     "You need all six keys to open this door"
#define STSTR_COMPON    "Compatibility Mode On"            // phares
#define STSTR_COMPOFF   "Compatibility Mode Off"           // phares

#endif

#endif
//-----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:09  fraggle
// Initial revision
//
//
//-----------------------------------------------------------------------------
