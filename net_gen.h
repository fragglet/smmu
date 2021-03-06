// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
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
// Misc. Networking functions
//
// Functions used by both server and client
//
//---------------------------------------------------------------------------

#ifndef NET_SEND

#define NET_SEND

#include "doomdef.h"
#include "sv_serv.h"

void SendCompressedPacket(netnode_t *node, gamepacket_t *gp);
gamepacket_t *DecompressPacket(compressedpacket_t *cp);

void SendPacket(netnode_t *node, netpacket_t *packet);
void SendGamePacket(netnode_t *node, gamepacket_t *gp);

netmodule_t *Net_ModuleForName(char *name);
netnode_t *Net_Resolve(char *name);

#endif /* NET_SEND */

//-------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:09  fraggle
// Initial revision
//
//
//-------------------------------------------------------------------------
