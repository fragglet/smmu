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
//  System specific network interface stuff.
//
//-----------------------------------------------------------------------------

#ifndef __I_NET__
#define __I_NET__

// Called by D_DoomMain.

void I_InitNetwork (void);
void I_NetCmd (void);

#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-07-29 13:20:41  fraggle
// Initial revision
//
// Revision 1.3  1998/05/16  09:52:27  jim
// add temporary switch for Lee/Stan's code in d_net.c
//
// Revision 1.2  1998/01/26  19:26:56  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:08  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
