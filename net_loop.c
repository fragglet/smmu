// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
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
// Loopback net module
//
// The loopback module is actually two seperate modules: one for the
// server and one for the client. The two communicate with each other
// as the two communication modules would do over a real network.
//
// ie.
//           [SERVER]                      [CLIENT]
//              |                              |
//   [LOOPBACK SERVER MODULE] - - - - [LOOPBACK CLIENT MODULE]
//
// By Simon Howard
//
//---------------------------------------------------------------------------

#include "doomdef.h"
#include "z_zone.h"

#include "sv_serv.h"

#define QUEUESIZE 128

// input queue for server

netpacket_t server_inqueue[QUEUESIZE];
int server_head, server_tail;

// input queue for client

netpacket_t client_inqueue[QUEUESIZE];
int client_head, client_tail;

//===========================================================================
//
// Server Module
//
//===========================================================================

static boolean Loopback_Server_Init();
static void Loopback_Server_Shutdown();
static void *Loopback_Server_GetPacket(int *node);
static void Loopback_Server_SendPacket(int node, void *data, int datalen);
static void Loopback_Server_SendBroadcast(void *data, int datalen);

netmodule_t loopback_server =
  {
    "loopback",
    Loopback_Server_Init,
    Loopback_Server_Shutdown,
    Loopback_Server_SendPacket,
    Loopback_Server_SendBroadcast,
    Loopback_Server_GetPacket,
  };

//-------------------------------------------------------------------------
//
// Loopback_Server_SendPacket
//
// Send packet to local client
//

static void Loopback_Server_SendPacket(int node, void *data, int datalen)
{
  // add packet to inqueue for client
  memcpy(&client_inqueue[client_tail], data, datalen);

  client_tail = (client_tail + 1) % QUEUESIZE;
}

//-------------------------------------------------------------------------
//
// Loopback_Server_SendBroadcast
//

static void Loopback_Server_SendBroadcast(void *data, int datalen)
{
  Loopback_Server_SendPacket(0, data, datalen);
}

//-------------------------------------------------------------------------
//
// Loopback_Server_GetPacket
//
// Get packets from local client
//
  
static void *Loopback_Server_GetPacket(int *node)
{
  void *return_packet;
  
  // get latest packet from server inqueue

  // no packets ?
  if(server_head == server_tail)
    return NULL;

  // return packet

  return_packet = &server_inqueue[server_head];

  server_head = (server_head + 1) % QUEUESIZE;

  if(node)
    *node = 0;
  
  return return_packet;
}

//-------------------------------------------------------------------------
//
// Loopback_Server_Init
//
// Initialise module
//

static boolean Loopback_Server_Init()
{
  loopback_server.initted = true;
  loopback_server.numnodes = 1;
  
  return true;         // initted ok
}

//-------------------------------------------------------------------------
//
// Loopback_Server_Init
//
// Shutdown module
//

static void Loopback_Server_Shutdown()
{
}

//===========================================================================
//
// Server Module
//
//===========================================================================

static boolean Loopback_Client_Init();
static void Loopback_Client_Shutdown();
static void *Loopback_Client_GetPacket(int *node);
static void Loopback_Client_SendPacket(int node, void *data, int datalen);
static void Loopback_Client_SendBroadcast(void *data, int datalen);

netmodule_t loopback_client =
  {
    "loopback",
    Loopback_Client_Init,
    Loopback_Client_Shutdown,
    Loopback_Client_SendPacket,
    Loopback_Client_SendBroadcast,
    Loopback_Client_GetPacket,
  };

//--------------------------------------------------------------------------
//
// Loopback_Client_SendPacket
//
// Send packet to local server
//

static void Loopback_Client_SendPacket(int node, void *data, int datalen)
{
  //  C_Printf("Loopback_Client_SendPacket\n");

  // add packet to inqueue for server
  memcpy(&server_inqueue[server_tail], data, datalen);

  server_tail = (server_tail + 1) % QUEUESIZE;
}

//--------------------------------------------------------------------------
//
// Loopback_Client_SendBroadcast
//
// Broadcast send -- same as normal send
//

static void Loopback_Client_SendBroadcast(void *data, int datalen)
{
  Loopback_Client_SendPacket(0, data, datalen);
}

//--------------------------------------------------------------------------
//
// Loopback_Client_GetPacket
//
// Get packet from local server
//
  
static void *Loopback_Client_GetPacket(int *node)
{
  void *return_packet;

  // get latest packet from client inqueue

  // no packets ?
  if(client_head == client_tail)
    return NULL;

  // return packet

  return_packet = &client_inqueue[client_head];

  client_head = (client_head + 1) % QUEUESIZE;

  if(node)
    *node = 0;
  
  return return_packet;
}

//--------------------------------------------------------------------------
//
// Loopback_Client_SendPacket
//
// Initialise module
//

static boolean Loopback_Client_Init()
{
  loopback_client.initted = true;
  loopback_client.numnodes = 1;

  return true;           // initted ok
}

//--------------------------------------------------------------------------
//
// Loopback_Client_Shutdown
//
// Shutdown module
//

static void Loopback_Client_Shutdown()
{
}

//-------------------------------------------------------------------------
//
// $Log$
// Revision 1.2  2000-06-19 14:57:14  fraggle
// make functions static
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//-------------------------------------------------------------------------
