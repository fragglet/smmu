// Emacs style mode select   -*- C++ -*-
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

static const char rcsid[] = "$Id$";

#include "version.h"

int VERSION = 321;        // sf: made int from define 
const char version_date[] = __DATE__;
const char version_name[] = "christmas"; // sf : version names
                                         // at the suggestion of mystican

// os type

doomos_t doomos_type = 
#ifdef DJGPP
os_dos;
#elif defined(LINUX)
os_linux;
#elif defined(_WIN32)
os_windows;
#else
os_unknown;
#endif


//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.3  2000-06-22 18:24:58  fraggle
// os_t -> doomos_t for peaceful coexistence with allegro
//
// Revision 1.2  2000/06/20 21:08:35  fraggle
// platform detection (dos, win32, linux etc)
//
// Revision 1.1.1.1  2000/04/30 19:12:08  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
