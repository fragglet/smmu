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
// Server Find
//
// We can search for servers by sending a packet of type pt_finger
// and waiting for a reply
//
//--------------------------------------------------------------------------

#include <stdio.h>

#include "c_io.h"
#include "c_runcmd.h"
#include "cl_clien.h"
#include "doomdef.h"
#include "mn_engin.h"
#include "net_gen.h"
#include "net_modl.h"
#include "sv_serv.h"
#include "v_misc.h"
#include "z_zone.h"

//==========================================================================
//
// Server List
//
// When we get replies to our ping requests we store the data
// returned in a list for further use
//
//=========================================================================

#define MAX_SERVERS 128

typedef struct
{
  netnode_t node;
  fingerpacket_t info;
} server_t;

static server_t servers[MAX_SERVERS];
static int num_servers;

//------------------------------------------------------------------------
//
// Cheap function to dump server list to console
//

static void Finger_DumpServerList()
{
  int i;

  C_Printf("location/accepting/players/name\n");
  
  for(i=0; i<num_servers; i++)
    {
      C_Printf
	("%s:%i/%s/%i/%s\n",
	 servers[i].node.netmodule->name, servers[i].node.node,
	 servers[i].info.accepting ? "yes" : "no",
	 servers[i].info.players,
	 servers[i].info.server_name);
    }
}

//==========================================================================
//
// Waiting for reply from servers
//
//==========================================================================

// time we wait for a reply from the servers 
#define WAIT_TIME 35*5

//-------------------------------------------------------------------------
//
// Finger_WaitReply
//
// Wait for WAIT_TIME tics and store replies from any servers in list
//

static void Finger_WaitReply(netmodule_t *module)
{
  boolean initted = module->initted;
  int endtime;
  
  // initialise module if neccesary
  
  if(!initted)
    {
      if(!module->Init())
	C_Printf("finger: error initting %s\n", module->name);
      return;
    }

  // wait for responses

  num_servers = 0;
  
  endtime = I_GetTime_RealTime() + WAIT_TIME;

  while(I_GetTime_RealTime() < endtime && num_servers < MAX_SERVERS)
    {
      netpacket_t *packet;
      int node;
      
      // update server for loopback reply
      
      SV_Update();
      
      // get packet
      
      packet = module->GetPacket(&node);
      if(!packet)
	continue;

      // check a finger reply

      if(packet->type != pt_finger)
	continue;

      // add server to list
      
      servers[num_servers].node.netmodule = module;
      servers[num_servers].node.node = node;
      servers[num_servers++].info = packet->data.u.fingerpacket;
    }
  
  // clean up -
  // shutdown module if it was not initialised at start
  
  if(!initted)
    module->Shutdown();    
}

//--------------------------------------------------------------------------
//
// Finger_WaitSingle
//
// Wait for a reply from a single server
//

static server_t *Finger_WaitSingle(netmodule_t *module)
{
  boolean initted = module->initted;
  static server_t returndata;
  int endtime;
  
  // initialise module if neccesary
  
  if(!initted)
    {
      if(!module->Init())
	C_Printf("finger: error initting %s\n", module->name);
      return NULL;
    }

  // wait for responses

  num_servers = 0;
  
  endtime = I_GetTime_RealTime() + WAIT_TIME;

  while(I_GetTime_RealTime() < endtime && num_servers < MAX_SERVERS)
    {
      netpacket_t *packet;
      int node;

      // update server

      SV_Update();
      
      // get packet
      
      packet = module->GetPacket(&node);
      if(!packet)
	continue;

      // check a finger reply

      if(packet->type != pt_finger)
	continue;

      returndata.node.netmodule = module;
      returndata.node.node = node;
      returndata.info = packet->data.u.fingerpacket;

      servers[num_servers++] = returndata;
      
      if(!initted)
	module->Shutdown();

      return &returndata;
    }
  
  // clean up -
  // shutdown module if it was not initialised at start
  
  if(!initted)
    module->Shutdown();

  return NULL;
}

//=========================================================================
//
// Finger Send
//
//=========================================================================

//-------------------------------------------------------------------------
//
// Finger_SendRequest
//
// Just sends a finger packet
//

static void Finger_SendRequest(netnode_t *node)
{
  netpacket_t packet;

  packet.type = pt_fingerrequest;

  SendPacket(node, &packet);
}

//-------------------------------------------------------------------------
//
// Finger_SendBroadcast
//
// Sends broadcast finger packet
//

static void Finger_SendBroadcast(netmodule_t *module)
{
  netpacket_t packet;
  netnode_t dest;

  packet.type = pt_fingerrequest;

  dest.node = NODE_BROADCAST;
  dest.netmodule = module;

  SendPacket(&dest, &packet);
}

//-------------------------------------------------------------------------
//
// CL_Finger
//
// Finger a server and wait for a reply
//

void CL_Finger(netnode_t *node)
{
  boolean initted;           // nodes module initted
  server_t *data;

  // init nodes netmodule if neccesary
  // save the previous value so we know if to shut it down afterwards
  
  initted = node->netmodule->initted;

  if(!initted)
    {
      if(!node->netmodule->Init())
	{
	  C_Printf("finger: could not init %s\n", node->netmodule->name);
	  return;
	}
    }

  // send packet
  
  Finger_SendRequest(node);

  // wait for reply

  data = Finger_WaitSingle(node->netmodule);

  if(data)
    {
      C_Printf("response:\n");
      C_Printf("server name: %s\n", data->info.server_name);
      C_Printf("players connected: %i\n", data->info.players);
      if(data->info.accepting)
	C_Printf("server is waiting for players\n"); 
    }
  else
    C_Printf("no reply.\n");
    
  // shutdown module if we initted it
  
  if(!initted)
    node->netmodule->Shutdown();
}

//--------------------------------------------------------------------------
//
// CL_BroadcastFinger
//
// Broadcast finger to all nodes on a particular module
// Useful for some modules eg. external, udp(lan)
//

void CL_BroadcastFinger(netmodule_t *module)
{
  boolean initted;

  // if module not initialised, initialise it now
  // remember if it we initted it so we can shut it down

  initted = module->initted;
  
  if(!initted)
    {
      if(!module->Init())
	{
	  C_Printf("finger: could not init %s\n", module->name);
	  return;
	}
    }

  if(menuactive)
    V_SetLoading(0, "looking for games");
  
  // broadcast finger packet

  Finger_SendBroadcast(module);
  
  // wait for replies

  Finger_WaitReply(module);

  // cleanup - shutdown module if we initted it
  
  if(!initted)
    module->Shutdown();
}

CONSOLE_COMMAND(finger, cf_buffered)
{
  if(!c_argc)
    {
      C_Printf("usage: finger <hostname>\n");
    }
  else
    {
      netnode_t *node;

      node = Net_Resolve(c_argv[0]);

      if(node)
	{
	  CL_Finger(node);
	}
      else
	C_Printf("unknown location\n");
    }
}

// broadcast finger

CONSOLE_COMMAND(finger2, 0)
{
  if(c_argc)
    {
      netmodule_t *module = Net_ModuleForName(c_argv[0]);

      if(!module)
	C_Printf("unknown module\n");
      else
	{
	  CL_BroadcastFinger(module);
	  Finger_DumpServerList();
	}
    }
  else
    {
      C_Printf("usage: finger2 <interface>\n");
    }
}

//==========================================================================
//
// Finger Reply Menu
//
// Function to build a menu containing the results of a finger
// send. User can view a list of servers and select one to join
// it.
//
//==========================================================================

menu_t finger_menu =
  {
    {},                    // items created by function
    30, 15,
    mf_background|mf_leftaligned,                // full screen
  };

void Finger_ServersMenu()
{
  int i = 0;
  int n;
  
  // build menu

  finger_menu.menuitems[i].type = it_title;
  finger_menu.menuitems[i++].description = FC_GOLD "servers";

  finger_menu.menuitems[i++].type = it_gap;

  finger_menu.menuitems[i].type = it_runcmd;
  finger_menu.menuitems[i].description = "<- back";
  finger_menu.menuitems[i++].data = "mn_prevmenu";

  finger_menu.menuitems[i++].type = it_gap;

  // if no servers, say so
  
  if(!num_servers)
    {
      finger_menu.menuitems[i].type = it_info;
      finger_menu.menuitems[i++].description = "no servers found.";
    }
  
  // run through twice
  // place the servers accepting connections at the top
  
  for(n=0; n<num_servers; n++)
    {
      if(servers[n].info.accepting)
	{
	  char tempstr[128];

	  // type
	  
	  finger_menu.menuitems[i].type = it_runcmd;

	  // description
	  
	  sprintf(tempstr, "* "FC_GREEN "%s (%i)",
		  servers[n].info.server_name,
		  servers[n].info.players);
      
	  finger_menu.menuitems[i].description
	    = Z_Strdup(tempstr, PU_STATIC, 0);

	  // command
	  
	  sprintf(tempstr, "connect %s:%i",
		  servers[n].node.netmodule->name,
		  servers[n].node.node);

	  finger_menu.menuitems[i++].data
	    = Z_Strdup(tempstr, PU_STATIC, 0);
	}
    }

  for(n=0; n<num_servers; n++)
    {
      if(!servers[n].info.accepting)
	{
	  char tempstr[128];
      
	  finger_menu.menuitems[i].type = it_runcmd;

	  // description
	  
	  sprintf(tempstr, "%s (%i)",
		  servers[n].info.server_name,
		  servers[n].info.players);

	  finger_menu.menuitems[i].description
	    = Z_Strdup(tempstr, PU_STATIC, 0);

	  // command
	  
	  sprintf(tempstr, "connect %s:%i",
		  servers[n].node.netmodule->name,
		  servers[n].node.node);

	  finger_menu.menuitems[i++].data
	    = Z_Strdup(tempstr, PU_STATIC, 0);
	}
    }
  
  finger_menu.menuitems[i].type = it_end;

  MN_StartMenu(&finger_menu);
}

CONSOLE_COMMAND(finger_menu, 0)
{
  Finger_ServersMenu();
}

#ifdef TCPIP

//========================================================================
//
// Internet Server Lookup
//
// Somewhere on the internet we have a computer running smmuserv
// which we can use to register new servers etc.
//
//========================================================================

#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

#ifndef DJGPP
#define lsck_perror perror
#define lsck_strerror strerror
#endif 

#define SMMUSERV_PORT 52638

char *server_location;

// "x-homepc";
// "212.250.184.229";

//==========================================================================
//
// Server List
//
// We store the list of servers we retrieve from smmuserv
//
//==========================================================================

static int inet_numservers;
static int inet_listsize;
static char **inet_serverlist;

static void Inet_ClearServers()
{
  int i;

  // free all data alloced to list
  
  if(inet_serverlist)
    {
      for(i=0; i<inet_numservers; i++)
	{
	  Z_Free(inet_serverlist[i]);
	}
      Z_Free(inet_serverlist);
    }

  // alloc new space
  
  inet_numservers = 0;
  inet_listsize = 128;
  inet_serverlist =
    Z_Malloc(sizeof(*inet_serverlist) * (inet_listsize+2), PU_STATIC, 0);
}

// add server to list

static void Inet_AddServer(char *location)
{
  // realloc bigger if we run out of space
  
  if(inet_numservers >= inet_listsize)
    {
      inet_listsize *= 2;
      inet_serverlist =
	Z_Realloc(inet_serverlist,
		  sizeof(*inet_serverlist) * (inet_listsize+2),
		   PU_STATIC, 0);
    }
  
  inet_serverlist[inet_numservers++] = Z_Strdup(location, PU_STATIC, 0);
}

//==========================================================================
//
// Basic Server Functions
//
// Connect, read and send lines etc.
//
//==========================================================================

//--------------------------------------------------------------------------
//
// strclean
//
// Remove unprintable characters from a string
//

static void strclean(char *s)
{
  char *p;

  for(p=s;*s; s++)
    if(isprint(*s) || *s == '\n')
      *p++ = *s;

  *p = '\0';
}

//--------------------------------------------------------------------------
//
// Inet_ReadLine
//
// Read a line from the server
//

#define BUFFER_SIZE 1024

static char read_buffer[BUFFER_SIZE] = "";

static char *Inet_ReadLine(int sock)
{
  char *nl;
  char *write_point;
  int bytes;

  if((nl = strchr(read_buffer, '\n')))
    {
      static char return_buffer[BUFFER_SIZE];
      
      strncpy(return_buffer, read_buffer, nl-read_buffer);
      return_buffer[nl-read_buffer] = '\0';
      
      strcpy(read_buffer, nl+1);
      
      return return_buffer;
    }

  // read new data
  // sf: typical windows crap.
  //     the winsock interface gets upset if we try to read in too many
  //     chars from the socket at once. Which means we have to read in
  //     the new data byte by byte to ensure that it actually works
  //     properly. This is not very nice at all, but it works.
  
  while(true)
    {
      write_point = read_buffer + strlen(read_buffer);
      bytes = read(sock, write_point, 1);

      if(bytes == -1)
	{
	  if(errno == EWOULDBLOCK)
	    break;
	  usermsg("receive error:\n%s\n", lsck_strerror(errno));
	}
      
      if(!bytes)
	break;
      
      write_point[bytes] = '\0';

      // clean new data
      
      strclean(write_point);
    }

  return NULL;
}

//--------------------------------------------------------------------------
//
// Write_Line
//
// Write a line to the server
//

static void Inet_WriteLine(int sock, char *line)
{
  //  char buffer[BUFFER_SIZE];
  //  char *s;
  
  // copy to buffer and add \n

  //  C_Printf(line);
  if(!write(sock, line, strlen(line) + 1))
    {
      usermsg("send error:\n%s\n", lsck_strerror(errno));
    }
}

//--------------------------------------------------------------------------
//
// Connect to server
//
// Returns the new socket or -1 if could not connect
//

static int Inet_ConnectServer(char *location)
{
  struct hostent *hent;
  struct sockaddr_in in;
  int sock;

  if(!tcpip_support)
    {
      C_Printf("no tcp/ip support.\n");
      return -1;
    }
  
  // display a "contacting" message in menu mode

  if(menuactive)
    V_SetLoading(0, "contacting server");
  else
    C_Printf("contacting server\n");
  
  // resolve server first
  
  hent = gethostbyname(location);
  if (hent == NULL)
    {
      C_Printf("unable to resolve smmuserv server\n");
      return -1;
    }

  // open socket and connect
  
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  in.sin_family = AF_INET;
  in.sin_addr.s_addr = ((struct in_addr *) hent->h_addr)->s_addr;
  in.sin_port = htons(SMMUSERV_PORT);

  if(connect(sock, (struct sockaddr *)&in, sizeof(in)) == -1)
    {
      C_Printf("unable to connect to server:\n"
	       "%s\n", lsck_strerror(errno));
      close(sock);
      return -1;
    }

  // non-blocking

  fcntl(sock, F_SETFL, O_NONBLOCK);
  
  // read message of the day

  while(true)
    {
      char *temp = NULL;

      // get a line

      while(!temp)
	temp = Inet_ReadLine(sock);

      if(!strcasecmp(temp, "ok"))          // 'ok' marks end of motd
	break;

      usermsg(FC_GRAY "%s", temp);
    }
  
  return sock;
}

//==========================================================================
//
// Contact SMMUSERV
//
// Add to list/retrieve server list
//
//==========================================================================

//--------------------------------------------------------------------------
//
// Register ourselves with smmuserv
//

static void Inet_RegisterServer()
{
  int sock;
  char *response;
  
  // connect to server
  
  sock = Inet_ConnectServer(server_location);
  if(sock == -1)
    return;

  // send add command

  Inet_WriteLine(sock, "add\n");

  // wait for response

  response = NULL;
  
  while(!response)
    {
      response = Inet_ReadLine(sock);
    }

  if(strcasecmp(response, "ok"))
    {
      C_Printf("%s\n", response);
    }
  else
    C_Printf("registered with server\n");
  
  close(sock);
}

//-------------------------------------------------------------------------
//
// Inet_GetServers
//
// Retrieve Server List
// returns true if list successfully retrieved
//

#define TIMEOUT 5

static boolean Inet_GetServers()
{
  int sock;
  int endtime;
  
  // clear server list
  
  Inet_ClearServers();

  // connect to server
  
  sock = Inet_ConnectServer(server_location);
  if(sock == -1)
    return false;

  // send command to list servers

  Inet_WriteLine(sock, "list\n");
  
  // get response

  endtime = I_GetTime_RealTime() + TIMEOUT*35;
  
  while(I_GetTime_RealTime() < endtime)
    {
      char *response;

      response = Inet_ReadLine(sock);

      if(!response)
	continue;

      // delay our timeout a bit longer
      
      endtime = I_GetTime_RealTime() + TIMEOUT*35;
      
      if(!strcmp(response, "ok"))    // end of list
	break;
      
      // add to server list

      Inet_AddServer(response);
    }

  if(I_GetTime_RealTime() >= endtime)
    {
      C_Printf("list incomplete\n");
    }
  
  close(sock);

  return true;
}

//--------------------------------------------------------------------------
//
// Inet_DumpServers
//
// Dump the list of servers we have retrieved
//

static void Inet_DumpServers()
{
  int i;

  if(!inet_serverlist || inet_numservers == 0)
    return;

  C_Printf("internet server list:\n");
  
  for(i=0; i<inet_numservers; i++)
    {
      C_Printf("%s\n", inet_serverlist[i]);
    }
}

//--------------------------------------------------------------------------
//
// Inet_FingerServers
//
// Retrieve server list from smmuserv and send finger packets to
// all of the servers listed
//

static void Inet_FingerServers()
{
  boolean initted;
  int i;
  
  // do nothing if we have no servers
  
  if(!inet_numservers)
    return;

  // init udp
  
  initted = udp.initted;
  if(!initted)
    {
      if(!udp.Init())
	return;
    }

  // send finger requests to servers on list

  if(menuactive)
    V_SetLoading(0, "fingering games");
  else
    C_Printf("fingering games\n");

  for(i=0; i<inet_numservers; i++)
    {
      netnode_t node;

      node.netmodule = &udp;
      node.node = UDP_Resolve(inet_serverlist[i]);

      if(node.node == -1)
	continue;

      Finger_SendRequest(&node);
    }

  // get replies

  Finger_WaitReply(&udp);
  
  // shutdown udp if it was not initialised originally
  
  if(!initted)
    udp.Shutdown();
}

//--------------------------------------------------------------------------
//
// Console Commands
//

CONSOLE_COMMAND(inet_add, 0)
{
  Inet_RegisterServer();
}

CONSOLE_COMMAND(inet_get, 0)
{
  Inet_GetServers();
  Inet_DumpServers();
}

CONSOLE_STRING(inet_server, server_location, NULL, 25, 0) {}

// find servers on a particular netmodule, then display the
// results in a menu

CONSOLE_COMMAND(find_servers, 0)
{
  if(!c_argc)
    {
      C_Printf("usage: find_servers <interface>\n");
      return;
    }

  // contact server and get ip list if internet
  
  if(!strcasecmp(c_argv[0], "internet"))
    {
      if(!Inet_GetServers())
	{
	  MN_ErrorMsg("unable to contact server");
	  return;
	}

      // contact servers on list
      
      Inet_FingerServers();
    }
  else
    {
      netmodule_t *module;
      
      module = Net_ModuleForName(c_argv[0]);
      if(!module)
	{
	  C_Printf("unknown module\n");
	  return;
	}

      // broadcast finger packet
      
      CL_BroadcastFinger(module);
    }

  // display menu of results
  
  Finger_ServersMenu();
}

//==========================================================================
//
// Inet_Init
//
// Called at startup. Load smmuserv servers list
//
//==========================================================================

#define MAX_SERVERS 128
#define SERVERS_FILE "servers"

char *smmuserv_servers[MAX_SERVERS];
int num_smmuserv_servers = 0;

void Inet_Init()
{
  FILE *servers_file;
  char buffer[128];
  int i = 0;
  
  servers_file = fopen(SERVERS_FILE, "r");

  if(!servers_file)
    {
      smmuserv_servers[num_smmuserv_servers = 0] = NULL;
      return;
    }

  while(!feof(servers_file))
    {
      fgets(buffer, 126, servers_file);

      while(buffer[0] == ' ')        // cut off starting spaces
	strcpy(buffer, buffer+1);

      if(!buffer[0] ||               // empty line or comment
	 buffer[0] == '#' || buffer[0] == ';' ||
	 (buffer[0] == '/' && buffer[1] == '/'))
	continue;

      smmuserv_servers[i++] = strdup(buffer);
    }

  num_smmuserv_servers = i;  
  smmuserv_servers[i] = NULL; // end
  
  fclose(servers_file);
}


#endif /* TCPIP */

//==========================================================================
//
// Console Commands
//
//==========================================================================

void Finger_AddCommands()
{
  C_AddCommand(finger);
  C_AddCommand(finger2);
  C_AddCommand(find_servers);
#ifdef TCPIP
  C_AddCommand(inet_add);
  C_AddCommand(inet_get);
  C_AddCommand(inet_server);
#endif
  C_AddCommand(finger_menu);
}

//--------------------------------------------------------------------------
//
// $Log$
// Revision 1.2  2000-06-04 17:19:02  fraggle
// easier reliable-packet send interface
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//--------------------------------------------------------------------------
