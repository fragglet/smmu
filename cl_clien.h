// Emacs style mode select -*- C++ -*-
//--------------------------------------------------------------------------
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
// Networking, Client Portion
//
// By Simon Howard
//
//--------------------------------------------------------------------------

#ifndef CL_CLIEN
#define CL_CLIEN

#include "d_event.h"
#include "sv_serv.h"

void CL_Connect(netnode_t *netnode);
void CL_Disconnect(char *reason);
void CL_LoopbackConnect();                 // connect to local server

void TryRunTics();
void NetUpdate();
void NetDisconnect(char *reason);

void CL_Finger(netnode_t *netnode);

void ResetNet();

void CL_WaitDrawer();
boolean CL_WaitResponder(event_t *ev);

extern int controller;

extern boolean opensocket;
extern boolean isconsoletic;
extern int ticdup;

#endif /* CL_CLIEN */
