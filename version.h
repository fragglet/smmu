// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
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
// $Id$
//
//-----------------------------------------------------------------------------


#ifndef __DOOMVERSION__
#define __DOOMVERSION__

//#include "z_zone.h"  /* memory allocation wrappers -- killough */

// DOOM version
// enum { VERSION =  203 };
extern int VERSION;     // sf: made version an int

extern const char version_date[];
extern const char version_name[];

extern const char version_os[];

#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.4  2000-08-16 13:29:14  fraggle
// more generalised os detection
//
// Revision 1.3  2000/06/22 18:24:59  fraggle
// os_t -> doomos_t for peaceful coexistence with allegro
//
// Revision 1.2  2000/06/20 21:08:35  fraggle
// platform detection (dos, win32, linux etc)
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
