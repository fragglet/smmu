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
// Generalised network functions used by both server and client
//
//---------------------------------------------------------------------------

#ifndef DEDICATED
#include "c_io.h"
#include "c_runcmd.h"
#include "m_random.h"
#include "z_zone.h"
#endif

#include "doomdef.h"
#include "net_gen.h"
#include "net_modl.h"
#include "sv_serv.h"

#define COMPRESSION

//==========================================================================
//
// Tic Compression
//
// We compress the tics we send across the network to reduce bandwidth
// consumption.
//
//==========================================================================

#define CPHEADER ((int) &((compressedpacket_t *) 0)->data)

#define CONSDATA_SHIFT 5

enum
  {
    cp_forward = 1,
    cp_side = 2,
    cp_updown = 4,
    cp_angle = 8,
    cp_buttons = 16,
    cp_consdata = (32 + 64 + 128),
  };

int uncompressed_length = 0;
int compressed_length = 0;

void SendCompressedPacket(netnode_t *netnode, gamepacket_t *gp)
{
  netpacket_t packet;
  compressedpacket_t *cp = &packet.data.compressed;
  int i, n;
  byte *datapos;
  
  cp->num_tics = gp->num_tics;

  // read out each of the tics and compress
  
  datapos = &cp->data[0];

  for(i=0; i<gp->num_tics; i++)
    {
      byte *controlbyte;
      
      // write player and ticnum
      
      *datapos++ = gp->tics[i].player;
      *datapos++ = gp->tics[i].ticnum;

      *datapos++ = gp->tics[i].ticcmd.consistancy;
	
      
      controlbyte = datapos++;
      *controlbyte = 0;

      if(gp->tics[i].ticcmd.forwardmove)
	{
	  *controlbyte |= cp_forward;
	  *datapos++ = gp->tics[i].ticcmd.forwardmove;
	}
      if(gp->tics[i].ticcmd.sidemove)
	{
	  *controlbyte |= cp_side;
	  *datapos++ = gp->tics[i].ticcmd.sidemove;
	}
      if(gp->tics[i].ticcmd.updownangle)
	{
	  *controlbyte |= cp_updown;
	  *datapos++ = gp->tics[i].ticcmd.updownangle;
	}
      if(gp->tics[i].ticcmd.angleturn)
	{
	  *controlbyte |= cp_angle;
	  *datapos++ = gp->tics[i].ticcmd.angleturn & 255;
	  *datapos++ = (gp->tics[i].ticcmd.angleturn >> 8) & 255;
	}
      if(gp->tics[i].ticcmd.buttons)
	{
	  *controlbyte |= cp_buttons;
	  *datapos++ = gp->tics[i].ticcmd.buttons;
	}

      // console data

      for(n=0; n<CONS_BYTES; n++)
	{
	  if(!gp->tics[i].ticcmd.consdata[n])
	    break;
	  *datapos++ = gp->tics[i].ticcmd.consdata[n];
	}
      *controlbyte |= ( n << CONSDATA_SHIFT);
    }

  packet.type = pt_compressed;
  
  // send to node

  //  C_Printf("compressed len: %i\n", datapos - &cp->data[0]);

  compressed_length += HEADERLEN + CPHEADER + (datapos - &cp->data[0]);
  uncompressed_length += GAMEHEADERLEN + gp->num_tics * sizeof(tic_t);
    
  netnode->netmodule->SendPacket
    (
     netnode->node,
     &packet,
     HEADERLEN + CPHEADER + (datapos - &cp->data[0])
     );
}

gamepacket_t *DecompressPacket(compressedpacket_t *cp)
{
  static gamepacket_t decompressed;
  byte *datapos;
  int i;
  int n;
  
  decompressed.num_tics = cp->num_tics;
  datapos = &cp->data[0];
  
  for(i=0; i<cp->num_tics; i++)
    {
      byte control;

      decompressed.tics[i].player = *datapos++;
      decompressed.tics[i].ticnum = *datapos++;

      decompressed.tics[i].ticcmd.consistancy = *datapos++;
      
      control = *datapos++;

      if(control & cp_forward)
	{
	  decompressed.tics[i].ticcmd.forwardmove = *datapos++;
	}
      else
	decompressed.tics[i].ticcmd.forwardmove = 0;
      
      if(control & cp_side)
	{
	  decompressed.tics[i].ticcmd.sidemove = *datapos++;
	}
      else
	decompressed.tics[i].ticcmd.sidemove = 0;

      if(control & cp_updown)
	{
	  decompressed.tics[i].ticcmd.updownangle = *datapos++;
	}
      else
	decompressed.tics[i].ticcmd.updownangle = 0;

      if(control & cp_angle)
	{
	  decompressed.tics[i].ticcmd.angleturn = *datapos++;
	  decompressed.tics[i].ticcmd.angleturn += (*datapos++) << 8;
	}
      else
	decompressed.tics[i].ticcmd.angleturn = 0;
      
      if(control & cp_buttons)
	{
	  decompressed.tics[i].ticcmd.buttons = *datapos++;
	}
      else
	decompressed.tics[i].ticcmd.buttons = 0;

      for(n=0; n<(control >> CONSDATA_SHIFT); n++)
	decompressed.tics[i].ticcmd.consdata[n] = *datapos++;
      for(; n<CONS_BYTES; n++)
	decompressed.tics[i].ticcmd.consdata[n] = 0;      
    }

  return &decompressed;
}

//==========================================================================
//
// Packet Sending Functions
//
//==========================================================================

//--------------------------------------------------------------------------
//
// SendPacket
//
// Higher-level SendPacket
//

void SendPacket(netnode_t *node, netpacket_t *packet)
{
  int packet_len = HEADERLEN;       // always send header at least

  // add extra bytes to packet_len depending on packet type
  
  switch(packet->type)
    {
      // game packets
    case pt_gametics:               // game data
      packet_len +=
	GAMEHEADERLEN +
	packet->data.gamepacket.num_tics * sizeof(tic_t);
      break;
      
      // resend tics
    case pt_clticresend:
      packet_len += sizeof(clticresend_t);
      break;
      
    case pt_svticresend:
      packet_len += sizeof(svticresend_t);         // optimize
      break;
            
    case pt_ack:                    // packet ack
      packet_len += sizeof(ackpacket_t);
      break;
      
      // other packets
    case pt_quit:                   // player quit
      packet_len += sizeof(quitpacket_t);
	break;
	
    case pt_shutdown:               // shutdown server
      // no shutdownpacket_t
      break;
      
    case pt_finger:                 // finger request
      packet_len += sizeof(fingerpacket_t);
      break;
      
    case pt_chat:                     // chat
    case pt_textmsg:                // send a text message to clients
      packet_len += sizeof(msgpacket_t);
      break;
      
      // join packets
      
    case pt_join:                   // connect to server
      packet_len += sizeof(joinpacket_t);
      break;
      
    case pt_accept:                 // accept connection to server
      packet_len += sizeof(acceptpacket_t);
      break;
      
    case pt_deny:                   // deny connection
      packet_len += sizeof(denypacket_t);
      break;
      
    case pt_waitinfo:               // while-u-wait info
      packet_len += sizeof(waitinfo_t);
      break;
      
    case pt_startgame:              // signal to start game
      packet_len += sizeof(startgame_t);
      break;
      
    default:
      break;
    }

//    if(M_Random() < 96)
//      {
//        C_Printf("skip packet\n");
//        return;
//      }  

  if(node->node == NODE_BROADCAST)
    node->netmodule->SendBroadcast
      (
       packet,
       packet_len
       );
  else 
    node->netmodule->SendPacket
      (
       node->node,
       packet,
       packet_len
       );
}

//--------------------------------------------------------------------------
//
// SendGamePacket
//
// Send a gamepacket_t to a particular node
//

void SendGamePacket(netnode_t *node, gamepacket_t *gp)
{
  if(!gp->num_tics)
    return;

#ifdef COMPRESSION

  SendCompressedPacket(node, gp);

#else
  
  netpacket_t packet;

  packet.type = pt_gametics;
  packet.data.gamepacket = *gp;
  
  SendPacket(node, &packet);

#endif
}

//=========================================================================
//
// High-level Resolve
//
// Allows for console command lines like:
//
//        connect 192.0.0.11        (tcp/ip address)
//        connect doom.org          (internet location)
//        connect ext:1             (external driver)
//        connect modem             (external driver -- serial)
//
//==========================================================================

//--------------------------------------------------------------------------
//
// Net_ModuleForName
//
// Given the name of a netmodule, return the actual one.
//

netmodule_t *Net_ModuleForName(char *module)
{
  // check netmodules
  // just use a load of if() statements

#ifdef TCPIP
  if(!strcasecmp(module, "tcpip") ||
     !strcasecmp(module, "udp") ||
     !strcasecmp(module, "internet") ||
     !strcasecmp(module, "net"))
    {
      return &udp;
    }
#endif

#ifndef DEDICATED /* none of these in dedicated server */

  if(!strcasecmp(module, "loopback") ||
     !strcasecmp(module, "loop"))
    {
      return &loopback_client;
    }
  
#ifdef DJGPP
  if(!strcasecmp(module, "external") ||
     !strcasecmp(module, "ext"))
    {
      return &external;
    }
  if(!strcasecmp(module, "ser") ||
     !strcasecmp(module, "serial"))
    {
      return &serial;
    }

  if(!strcasecmp(module, "modem"))
    {
      return &modem;
    }
#endif /* #ifdef DJGPP */

#endif /* #ifndef DEDICATED */

  return NULL;     // dont know
}

//---------------------------------------------------------------------------
//
// Net_Resolve
//
// Resolve a name to a netnode_t
//

netnode_t *Net_Resolve(char *name)
{
  char *module, *nodenum;
  char *seperator;
  static netnode_t resolved;
  netmodule_t *nm;
  
  seperator = strchr(name, ':');

  if(seperator)
    {
      // get module
      module = malloc(seperator - name + 4);
      strncpy(module, name, seperator-name);
      module[seperator-name] = '\0';
      
      // get nodenum
      nodenum = seperator + 1;
    }
  else
    {
      module = name;
      nodenum = "1";    // 1 is usually the first of the other nodes
    }

  // try to find the netmodule given
  
  nm = Net_ModuleForName(module);

  if(seperator)
    free(module);

  if(nm)
    {
      resolved.netmodule = nm;
      resolved.node = atoi(nodenum);
    }
  else
    {
#ifdef TCPIP

      int udp_node;
      
      // not a netmodule or not a valid one
      // try tcp/ip resolve

      udp_node = UDP_Resolve(name);

      if(udp_node >= 0)
	{
	  resolved.netmodule = &udp;
	  resolved.node = udp_node;
	}
      else
#endif
	{
	  // unknown location
	  return NULL;
	}
    }
  
  return &resolved;
}

//========================================================================
//
// Console Commands
//
//========================================================================

#ifndef DEDICATED /* no console in dedicated server */

void CL_AddCommands();           // cl_clien.c
void SV_AddCommands();           // sv_serv.c
void Finger_AddCommands();       // cl_find.c
void UDP_AddCommands();          // net_udp.c

CONSOLE_COMMAND(efficiency, 0)
{
  C_Printf("game packet send efficiency:\n");
  C_Printf("%i%%", (compressed_length*100)/uncompressed_length);
}

void net_AddCommands()
{
  C_AddCommand(efficiency);
  
  CL_AddCommands();
  SV_AddCommands();
  Finger_AddCommands();

#ifdef TCPIP
  UDP_AddCommands();
#endif
}

#endif /* #ifndef DEDICATED */
