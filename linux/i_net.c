// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_net.c,v 1.4 1998/05/16 09:41:03 jim Exp $
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
//
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

#include "../z_zone.h"  /* memory allocation wrappers -- killough */

static const char
rcsid[] = "$Id: i_net.c,v 1.4 1998/05/16 09:41:03 jim Exp $";

#include "../doomstat.h"
#include "../i_system.h"
#include "../d_event.h"
#include "../d_net.h"
#include "../m_argv.h"

#include "../i_net.h"

void    NetSend (void);
boolean NetListen (void);

//
// NETWORKING
//

void    (*netget) (void);
void    (*netsend) (void);

//
// PacketSend
//
void PacketSend (void)
{
}


//
// PacketGet
//
void PacketGet (void)
{
}

//
// I_InitNetwork
//
void I_InitNetwork (void)
{
  int                 i,j;

  // set up the singleplayer doomcom

  singleplayer.id = DOOMCOM_ID;
  singleplayer.numplayers = singleplayer.numnodes = 1;
  singleplayer.deathmatch = false;
  singleplayer.consoleplayer = 0;
  singleplayer.extratics=0;
  singleplayer.ticdup=1;
 
  // set up for network
                          
  // parse network game options,
  //  -net <consoleplayer> <host> <host> ...
  i = 0;        //M_CheckParm ("-net");
  if (!i)
  {
    // single player game
    doomcom = &singleplayer;
    netgame = false;
    return;
  }
}


void I_NetCmd (void)
{
  if (doomcom->command == CMD_SEND)
  {
    netsend ();
  }
  else if (doomcom->command == CMD_GET)
  {
    netget ();
  }
  else
      I_Error ("Bad net cmd: %i\n",doomcom->command);
}



//----------------------------------------------------------------------------
//
// $Log: i_net.c,v $
// Revision 1.4  1998/05/16  09:41:03  jim
// formatted net files, installed temp switch for testing Stan/Lee's version
//
// Revision 1.3  1998/05/03  23:27:19  killough
// Fix #includes at the top, nothing else
//
// Revision 1.2  1998/01/26  19:23:26  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:07  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
