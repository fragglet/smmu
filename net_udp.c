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
// UDP Networking module
//
// Standard BSD Sockets. Will work on linux or other unixite systems
// and also in DJGPP with libsocket
//
//---------------------------------------------------------------------------

// we only use the data in this file if we have
// tcp/ip enabled

#ifdef TCPIP 

#ifdef DJGPP
#include <lsck/lsck.h>
#else
#define lsck_perror perror
#define lsck_strerror strerror
#define __lsck_gethostname gethostname
#endif

#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

#ifndef DEDICATED
#include "c_io.h"
#include "c_runcmd.h"
#include "z_zone.h"
#else
#define C_Printf printf
#define usermsg puts
#endif

#include "doomdef.h"
#include "sv_serv.h"

boolean tcpip_support;

int udp_port = 46375;

static int receive_socket;
//static int send_socket;

//==========================================================================
//
// Init Library
//
//==========================================================================

void UDP_InitLibrary()
{
#ifdef DJGPP
  tcpip_support = false;

  // if windows loaded, load libsocket library
  
  if(I_DetectWin95())
    if(__lsck_init())
      tcpip_support = true;
#else
  tcpip_support = true;
#endif
}

//==========================================================================
//
// UDP Module
//
//==========================================================================

// prototypes:

boolean UDP_Init();
void UDP_Shutdown();
void UDP_SendPacket(int node, void *data, int datalen);
void UDP_SendBroadcast(void *data, int datalen);
void *UDP_GetPacket(int *node);
void *UDP_GetPacket_Lagged(int *node);

netmodule_t udp =
  {
    "udp",
    UDP_Init,
    UDP_Shutdown,
    UDP_SendPacket,
    UDP_SendBroadcast,
    UDP_GetPacket_Lagged,
  };

//==========================================================================
//
// Node lookup
//
// Each time we get a packet from a new address we need to allocate it
// a new node number. This deals with resolving addresses to node
// numbers.
//
//==========================================================================

static struct sockaddr_in *nodetable;             // node table
static int nodetable_mallocedsize = -1;

//-------------------------------------------------------------------------
//
// UDP_InitNodeTable
//
// Allocate the node table
//

static void UDP_InitNodeTable()
{
  // do not create table if it already exists
  
  if(nodetable_mallocedsize > 0)
    return;
  
  // alloc table
  
  nodetable_mallocedsize = 64;      // 64 is enough for a start
  
  nodetable = malloc(sizeof(*nodetable) * (nodetable_mallocedsize + 4));

  // no nodes yet

  udp.numnodes = 0;
}

//--------------------------------------------------------------------------
//
// UDP_AddNode
//
// Add a node to the node table. Realloc the node table bigger
// if we run out of space
// Returns the nodenum of the new node
//

static int UDP_AddNode(struct sockaddr_in *in)
{
  // realloc bigger if neccesary
  
  if(udp.numnodes >= nodetable_mallocedsize)
    {
      nodetable_mallocedsize *= 2;
      nodetable =
	realloc(nodetable,
	 sizeof(*nodetable) * (nodetable_mallocedsize + 4));
    }


  //  C_Printf("add %s\n", inet_ntoa(in->sin_addr));
  
  // add to list
  
  nodetable[udp.numnodes] = *in;

  return udp.numnodes++; 
}
  
//--------------------------------------------------------------------------
//
// UDP_NodeForAddress
//
// Find the node number of a particular address
//

static int UDP_NodeForAddress(struct sockaddr_in *in)
{
  int i;

  if(!tcpip_support)
    return -1;
  
  // create nodetable if we have not yet done so
  
  if(nodetable_mallocedsize < 0)
    UDP_InitNodeTable();
  
  // search table for node
  // sequential find - ugh

  for(i=0; i<udp.numnodes; i++)
    {
      if(!memcmp(&in->sin_addr, &nodetable[i].sin_addr, sizeof(in->sin_addr)))
	{
	  nodetable[i] = *in;
	  return i;
	}
    }
  
  // node not yet in list
  // this is a new node
  // add it to the list
  
  return UDP_AddNode(in);
}

//==========================================================================
//
// Resolve address
//
// Resolve text format address to a node number
//
//==========================================================================

//--------------------------------------------------------------------------
//
// UDP_Resolve
//
// Resolve an address to a node number
// Returns -1 if could not resolve
//

int UDP_Resolve(char *location)
{
  struct hostent *host;
  struct sockaddr_in in;

  if(!tcpip_support)
    return -1;
  
  // resolve address
  
  if(!(host = gethostbyname(location)))
    {
      // could not resolve address
      return -1;
    }
  
  in.sin_family = AF_INET;
  in.sin_addr = *(struct in_addr *) host->h_addr;
  in.sin_port = htons(udp_port);

  return UDP_NodeForAddress(&in);
}

//==========================================================================
//
// Module Functions
//
//==========================================================================

//--------------------------------------------------------------------------
//
// UDP_Init
//
// Init UDP Module
//

// how many different port numbers to try
#define BIND_TRIES 100

boolean UDP_Init()
{
  struct sockaddr_in in;
  //  int one = 1;
  int i;

  if(!tcpip_support)
    {
      C_Printf("tcp/ip support unavailable\n");
      return false;
    }
  
  if(udp.initted)
    return true;

  //----------------------------------------------------------------
  //
  // Set up a receiving socket
  //
  
  receive_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if(receive_socket < 0)
    {
      C_Printf("could not create receive socket:\n%s\n", lsck_strerror(errno));
      return false;
    }

  // set nonblocking mode
  // keep the game running if we have no new packets
  
  fcntl(receive_socket, F_SETFL, O_NONBLOCK);
 
  // bind the socket to a port number
  // try up to 100 ports if one is taken

  for(i=0; i<BIND_TRIES; i++)
    {
      // set to receive packets from any address
      // as well as from players/server we want to
      // be able to reply to finger/ping requests
      
      memset(&in, 0, sizeof(in));
      in.sin_family = AF_INET;
      in.sin_addr.s_addr = INADDR_ANY;
      in.sin_port = htons(udp_port + i);

      // bind socket to address
      if(bind(receive_socket, (struct sockaddr *) &in, sizeof (in)) != -1)
	break;
    }
  
  if(i < BIND_TRIES)
    {
      if(i)
	C_Printf("bound to %i (%i taken)\n", udp_port+i, udp_port);
    }
  else
    {
      C_Printf("error binding receive socket:\n%s\n", lsck_strerror(errno));
      return false;
    }

  
  //----------------------------------------------------------------------
  //
  // Set up a sending socket
  //
  
  //  send_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  //  if(send_socket < 0)
  //    {
  //      C_Printf("error initting sending socket\n");
  //      return;
  //    }
  
  //----------------------------------------------------------------------
  //
  // Init other stuff
  //
  
  udp.initted = true;

  // init node-address lookup table

  UDP_InitNodeTable();
  
  // initted ok

  return true;
}

//-------------------------------------------------------------------------
//
// UDP_Shutdown
//
// Shutdown sockets, etc.
//

void UDP_Shutdown()
{
  if(!tcpip_support)
    return;

  //  C_Printf("udp shutdown\n");

  // close sockets

  close(receive_socket);
  //  close(send_socket);

  udp.initted = false;
}

//--------------------------------------------------------------------------
//
// UDP_SendPacket
//
// Send a packet to a node
//

void UDP_SendPacket(int nodenum, void *data, int datalen)
{
  if(!udp.initted || !tcpip_support)
    return;
  if(nodenum < 0 || nodenum >= udp.numnodes)
    return;

  sendto
    (
     receive_socket,
     data,
     datalen,
     0,
     (struct sockaddr *) &nodetable[nodenum],
     sizeof(struct sockaddr_in)
     );  
}

//--------------------------------------------------------------------------
//
// UDP_SendBroadcast
//
// Send broadcast packet
//

static boolean got_broadcast;
static struct in_addr broadcast;

static void UDP_GetBroadcast()
{
#ifdef DJGPP

  // libsocket packet broadcast is shaky.
  // To get address to send for broadcast
  // we take the local address (eg 192.0.0.11) and replace
  // the last digit with 255 (eg. 192.0.0.255) for local
  // broadcast

  char hostname[128];
  struct hostent *he;
  
  // get local hostname and resolve to find address

  __lsck_gethostname(hostname, 125);

  he = gethostbyname(hostname);

  if(he)
    {
      broadcast = *(struct in_addr *) he->h_addr;
      broadcast.s_addr |= (0xff << 24);  // change to *.*.*.255 - portable?
      got_broadcast = true;
    }

#else

  // everywhere else we just use the proper broadcast address

  if(!got_broadcast)
    {
      got_broadcast = true;
      broadcast.s_addr = INADDR_ANY;
    }

#endif /* #ifdef DJGPP */
}

void UDP_SendBroadcast(void *data, int datalen)
{
  struct sockaddr_in in;
  int i;
  
  if(!udp.initted || !tcpip_support)
    return;

  // get broadcast address
  
  if(!got_broadcast)
    {
      UDP_GetBroadcast();
      if(!got_broadcast)
	return;
    }
  
  in.sin_family = AF_INET;
  in.sin_addr = broadcast;
  in.sin_port = htons(udp_port);
  
  i = sendto
    (
     receive_socket,
     data,
     datalen,
     0,
     (struct sockaddr *) &in,
     sizeof(struct sockaddr_in)
     );

  if(!i)
    C_Printf("error sending broadcast:\n%s\n", lsck_strerror(errno));
}

//-------------------------------------------------------------------------
//
// UDP_GetPacket
//
// Get a new packet (if any)
//

void *UDP_GetPacket(int *node)
{
  int i;
  struct sockaddr_in src;
  int addr_len;
  static netpacket_t packet;

  if(!udp.initted || !tcpip_support)
    return NULL;
  
  // get any new packet
  
  addr_len = sizeof(src);
  
  i = recvfrom
    (
     receive_socket,
     &packet,
     sizeof(packet),
     0,
     (struct sockaddr *) &src,
     &addr_len
     );

  if(i < 0)
    {
      // error

      // return if blocking socket
      
      if(errno == EWOULDBLOCK)
	return NULL;

      C_Printf("socket receiving error:\n%s\n", lsck_strerror(errno));
      return NULL;
    }
  
  // find nodenum

  if(node)
    {
      *node = UDP_NodeForAddress(&src);
    }

  // pass back received packet
  
  return &packet;
}

//------------------------------------------------------------------------
//
// UDP_GetPacket_Lagged
//
// UDP_GetPacket but with simulated lag
//

#define INET_PACKETS 64
#define LAG 8
static netpacket_t inet_packets[INET_PACKETS];
static int inet_source[INET_PACKETS];
static int inet_gettime[INET_PACKETS];
static int inet_head, inet_tail;

void *UDP_GetPacket_Lagged(int *node)
{
  int i;
  struct sockaddr_in src;
  int addr_len;
  static netpacket_t packet;
    
  if(!udp.initted || !tcpip_support)
    return NULL;
  
  // get any new packet
  
  addr_len = sizeof(src);
  
  i = recvfrom
    (
     receive_socket,
     &packet,
     sizeof(packet),
     0,
     (struct sockaddr *) &src,
     &addr_len
     );

  if(i < 0 && errno != EWOULDBLOCK)
    {
      C_Printf("socket receiving error:\n%s\n", lsck_strerror(errno));
    }

  if(i >= 0)
    {
      // add to queue
      
      inet_packets[inet_head] = packet;
      inet_source[inet_head] = UDP_NodeForAddress(&src);
      inet_gettime[inet_head] = I_GetTime_RealTime();
      
      inet_head = (inet_head + 1) % INET_PACKETS;
    }
  
  if(inet_head == inet_tail)
    return NULL;
  
  if(I_GetTime_RealTime() > inet_gettime[inet_tail] + LAG)
    {
      i = inet_tail;
      inet_tail = (inet_tail + 1) % INET_PACKETS;
      
      if(node)
	*node = inet_source[i];
      
      return &inet_packets[i];
    }

  return NULL;
}

//==========================================================================
//
// Console Commands
//
//==========================================================================

#ifndef DEDICATED /* no console cmds in dedicated server */

CONSOLE_INT(udp_port, udp_port, NULL, 0, 65535, NULL, 0) {}

void UDP_AddCommands()
{
  C_AddCommand(udp_port);
}

#endif /* #ifndef DEDICATED */

#endif /* #ifdef TCPIP */

//-------------------------------------------------------------------------
//
// $Log$
// Revision 1.2  2000-05-02 15:43:10  fraggle
// lag simulation code
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//-------------------------------------------------------------------------
