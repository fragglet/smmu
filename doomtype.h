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
//      Simple basic typedefs, isolated here to make it easier
//       separating modules.
//
//-----------------------------------------------------------------------------


#ifndef __DOOMTYPE__
#define __DOOMTYPE__

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
// Fixed to use builtin bool type with C++.
#ifdef __cplusplus
typedef bool boolean;
#else
typedef enum {false, true} boolean;
#endif
typedef unsigned char byte;
#endif

#ifdef _WIN32

#undef MAXCHAR
#define MAXCHAR         ((char)0x7f)
#undef MAXSHORT
#define MAXSHORT        ((short)0x7fff)
#undef MAXINT
#define MAXINT          ((int)0x7fffffff)       
#undef MAXLONG
#define MAXLONG         ((long)0x7fffffff)

#undef MINCHAR
#define MINCHAR         ((char)0x80)
#undef MINSHORT
#define MINSHORT        ((short)0x8000)
#undef MININT
#define MININT          ((int)0x80000000)       
#undef MINLONG
#define MINLONG         ((long)0x80000000)

#else

#include <values.h>

#endif

#define MAXCHAR         ((char)0x7f)
#define MINCHAR         ((char)0x80)
#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.2  2000-06-19 14:58:55  fraggle
// cygwin (win32) support
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
