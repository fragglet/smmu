// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright(C) 2000 Simon Howard
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
//  Mission start screen wipe/melt, special effects.
//
//-----------------------------------------------------------------------------

#ifndef __F_WIPE_H__
#define __F_WIPE_H__

//
// SCREEN WIPE PACKAGE
//

void Wipe_Drawer();
void Wipe_Ticker();
void Wipe_StartScreen();

extern boolean inwipe;
extern int wipe_speed;

#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:09  fraggle
// Initial revision
//
//
//----------------------------------------------------------------------------
