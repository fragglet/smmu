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
// External driver net module
//
// For networking using the new networking system with the old
// external drivers.
//
// By Simon Howard
//
//----------------------------------------------------------------------------

#include <dpmi.h>
#include <sys/nearptr.h>

#include "../i_system.h"
#include "../m_argv.h"
#include "../m_random.h"
#include "../sv_serv.h"

#include "../doomdef.h"
#include "../z_zone.h"

// the following is from the original sources

// number to ensure correct ptr

#define DOOMCOM_ID              0x12345678l

// network driver commands: send and get packets

enum
  {
    CMD_SEND    = 1,
    CMD_GET     = 2
  };

// struct used for communication with driver

typedef struct
{
  // Supposed to be DOOMCOM_ID?
  long                id;
  
  // DOOM executes an int to execute commands.
  short               intnum;         
  // Communication between DOOM and the driver.
    // Is CMD_SEND or CMD_GET.
  short               command;
  // Is dest for send, set by get (-1 = no packet).
  short               remotenode;
  
  // Number of bytes in doomdata to be sent
  short               datalength;
  
  // Info common to all nodes.
  // Console is allways node 0.
  short               numnodes;
  // Flag: 1 = no duplication, 2-5 = dup for slow nets.
  short               ticdup;
  // Flag: 1 = send a backup tic in every packet.
  short               extratics;
  // Flag: 1 = deathmatch.
  short               deathmatch;
  // Flag: -1 = new game, 0-5 = load savegame
  short               savegame;
  short               episode;        // 1-3
  short               map;            // 1-9
  short               skill;          // 1-5
  
  // Info specific to this node.
  short               consoleplayer;
  short               numplayers;
  
  // These are related to the 3-display mode,
  //  in which two drones looking left and right
  //  were used to render two additional views
  //  on two additional computers.
  // Probably not operational anymore.
  // 1 = left, 0 = center, -1 = right
  short               angleoffset;
  // 1 = drone
  short               drone;          

  // The packet data to be sent.
  netpacket_t         data;  
} doomcom_t;

static doomcom_t *doomcom;

//-------------------------------------------------------------------------
//
// External_Call
//
// Call the driver to run a command for us
//

static void External_Call (void)
{
  __dpmi_regs r;                       
  __dpmi_int(doomcom->intnum,&r);
}

//--------------------------------------------------------------------------
//
// Driver Module
//

static boolean External_Init();
static void External_Shutdown();
static void External_SendPacket(int node, void *data, int datalen);
static void External_SendBroadcast(void *data, int datalen);
static void *External_GetPacket(int *node);

netmodule_t external =
  {
    "external driver",
    External_Init,
    External_Shutdown,
    External_SendPacket,
    External_SendBroadcast,
    External_GetPacket,
  };

//-------------------------------------------------------------------------
//
// External_Init
//
// Initialise the module
//

static boolean External_Init()
{
  int i;

  if(external.initted)
    return true;
  
  i = M_CheckParm ("-net");
  if (!i)
    return false;

  doomcom = (doomcom_t *)(__djgpp_conventional_base+atoi(myargv[i+1]));

  if(doomcom->id != DOOMCOM_ID)
    {
      I_Error("doomcom->id != DOOMCOM_ID");
    }

  external.initted = true;
  external.numnodes = doomcom->numnodes;
  
  return true;               // successfully initted
}

//--------------------------------------------------------------------------
//
// External_Shutdown
//
// Shut down the module
//

static void External_Shutdown()
{
}

//-------------------------------------------------------------------------
//
// External_GetPacket
//
// Get any new packets from the external driver
//

static void *External_GetPacket(int *node)
{
  static netpacket_t netpacket;
  
  if(!external.initted)
    return NULL;

  doomcom->command = CMD_GET;

  // call the driver

  External_Call();

  // any packet waiting?

  if(doomcom->remotenode == -1)
    return NULL;

  if(node)
    *node = doomcom->remotenode;

  memcpy(&netpacket, &doomcom->data, doomcom->datalength);
  
  return &netpacket;
}

//------------------------------------------------------------------------
//
// External_SendPacket
//
// Send a packet
//

static void External_SendPacket(int node, void *data, int datalen)
{
  if(!external.initted)
    return;
  
  //  C_Printf("External_SendPacket(%i)\n", node);

#if 0
  // test resend feature
  if(M_Random() < 64)
    {
      C_Printf("skip packet\n");
      return;
    }
#endif
  
  doomcom->command = CMD_SEND;
  doomcom->remotenode = node;
  doomcom->datalength = datalen;
  
  // copy data into doomcoom

  memcpy(&doomcom->data, data, datalen);

  // call driver to send

  External_Call();  
}

//---------------------------------------------------------------------------
//
// External_SendBroadcast
//
// Broadcast packet send
//

static void External_SendBroadcast(void *data, int datalen)
{
  int i;

  for(i=0; i<doomcom->numnodes; i++)
    External_SendPacket(i, data, datalen);
}

//---------------------------------------------------------------------------
//
// $Log$
// Revision 1.2  2000-06-19 14:57:18  fraggle
// make functions static
//
// Revision 1.1.1.1  2000/04/30 19:12:12  fraggle
// initial import
//
//
//---------------------------------------------------------------------------

