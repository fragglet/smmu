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
// Networking Server portion
//
// By Simon Howard
//
//---------------------------------------------------------------------------

/*********************************

 All the large comments like
 this are where I have not
 finished writing the code yet
  
*********************************/

#include <stdarg.h>
#include <time.h>

#ifdef DEDICATED

#define C_Printf printf
#define usermsg puts

#else /* dont need these for dedicated server */

#include "c_io.h"
#include "c_runcmd.h"
#include "c_net.h"
#include "cl_clien.h"
#include "mn_engin.h"
#include "z_zone.h"

#endif

#include "doomdef.h"
#include "net_gen.h"
#include "net_modl.h"
#include "v_misc.h"
#include "sv_serv.h"

static void SV_SendWaitInfo();

// name of server

char *server_name;

// all the nodes in a game

typedef struct
{
  netnode_t netnode;

  char name[20];
  int player;

  boolean drone;
  boolean waiting;           // true if waiting for game to start
  
  int latency;
  
  // save all previously sent packets in case
  // a packet is lost in send

  netpacket_t backup_packets[BACKUP_PACKETS];
  int backup_sendtime[BACKUP_PACKETS];    // time we sent packets
  int packet_sent;                        // packet we have sent
  int packet_acked;                       // packet client has acknowledged

  int packet_expected;               // packet we are expecting from client
  
  // outgoing tics

  gamepacket_t outque;
} nodeinfo_t;

static nodeinfo_t server_nodes[MAXNODES];
static int server_numnodes;

// player info

typedef struct
{ 
  int node;                        // node num of player
  boolean ingame;                  // if in game

  // tic cmd
  ticcmd_t ticcmds[256];           // received ticcmds
  int got_tic;                     // most recent tic number
  int sync_time;                   // time we got .ticcmds[0]

  boolean resend;                  // if true, send resend request
  int resend_time;                 // time we sent resend request for timeout
} playerinfo_t;

static playerinfo_t server_players[MAXPLAYERS];
static int server_numplayers;

// the source of the current packet being dealt with

static netnode_t source;
boolean server_active;

extern int extratics;    // how many previous tics to include in packets

// set to true when we are waiting for players to join the game

boolean waiting_players;

int sv_stack=1;          // stack height before sending

// sf: every now and again, we send the clients packets telling
// them how much to speed up (need to sync all the clients for the
// smoothest game)

// SYNC_COUNTDOWN_TIME is the number of cycles of player.ticcmds to wait
// before sending the speedup/slowdown packets. 
#define SYNC_COUNTDOWN_TIME 1
static int sync_countdown = SYNC_COUNTDOWN_TIME;

static long wad_signature;       // signature of wad players are using

//---------------------------------------------------------------------------
//
// SV_ServerNode
//
// Get the server_node the received packet came from
//

static int SV_ServerNode()
{
  int i;
  
  // find which server_node this packet is from
  
  for(i=0; i<server_numnodes; i++)
    {
      if(server_nodes[i].netnode.netmodule == source.netmodule &&
	 server_nodes[i].netnode.node == source.node)
	return i;
    }

  // packet not from a computer in the game?  
  return -1;
}

//---------------------------------------------------------------------------
//
// SV_FindController
//
// Find which node is the controller of the server. The controller is
// the first non-drone in the game. The controller global variable is
// set to the controlling node, or -1 if there is no controller yet.
//

static int controller;

static void SV_FindController()
{
  int i;

  for(i=0; i<server_numnodes; i++)
    if(!server_nodes[i].drone)
      break;

  controller = i == server_numnodes ? -1 : i;
}

//===========================================================================
//
// Reliable Packet Send
//
// Certain packets (console cmds, player quit etc) need to be sent
// "reliably" - ie. we need to know they have arrived. Use a system
// of acknowledging received packets by sending packets back to the
// server
//
//==========================================================================

//--------------------------------------------------------------------------
//
// SV_CheckResend
//
// Check if any packets sent to nodes have timed out and need to
// be resent
//

static void SV_CheckResend()
{
  int n;

  // check all nodes
  
  for(n=0; n<server_numnodes; n++)
    {
      nodeinfo_t *ni = &server_nodes[n];
      int p;

      for(p=ni->packet_acked; p!=ni->packet_sent; p = (p + 1) % BACKUP_PACKETS)
	{
	  //	  C_Printf("%i awaiting ack (%i %i)\n", p,
	  //		   ni->packet_acked, ni->packet_sent);
	  if(I_GetTime_RealTime() >
	     ni->backup_sendtime[p] + ni->latency)
	    {
	      // resend packet

	      //  C_Printf("resend packet %i to node %i\n", p, n);
	      SendPacket(&ni->netnode, &ni->backup_packets[p]);
	      ni->backup_sendtime[p] = I_GetTime_RealTime();
	    }
	}	
    }
}

//--------------------------------------------------------------------------
//
// SV_AckPacket
//
// Acknowledge that a packet we have sent to a client has arrived
//

static void SV_AckPacket(ackpacket_t *ack)
{
  int node;
  int delta;
  
  node = SV_ServerNode();
  if(node == -1)
    return;

  // check if already acked
  
  delta = ack->packet - server_nodes[node].packet_acked;
  if(delta > -64 && delta < 0)
    return;

  //  C_Printf("acked up to %i\n", ack->packet);
  
  server_nodes[node].packet_acked = (ack->packet + 1) % BACKUP_PACKETS;
}

//--------------------------------------------------------------------------
//
// SV_CorrectPacket
//
// Check packet we have received is the correct one
//

static boolean SV_CorrectPacket(int packet_num)
{
  int node;
  nodeinfo_t *ni;
  int delta;
  
  node = SV_ServerNode();
  if(node == -1)
    return false;

  ni = &server_nodes[node];

  delta = packet_num - ni->packet_expected;

  //  C_Printf("sv: delta %i\n", delta);
  
  // correct packet?

  if(delta == 0)
    ni->packet_expected = (packet_num + 1) % BACKUP_PACKETS;

  // send ack reply if it is for a packet we want or already have

  if(delta <= 0 && delta > -64)
    {
      netpacket_t reply;
      
      reply.type = pt_ack;
      reply.data.u.ackpacket.packet =
	(ni->packet_expected - 1) % BACKUP_PACKETS;

      //  C_Printf("sv: send ack to %i\n", reply.data.u.ackpacket.packet);
      
      SendPacket(&ni->netnode, &reply);
    }

  return delta == 0;
}

//--------------------------------------------------------------------------
//
// SV_ReliableSend
//
// Send packet to client, but store in backup_packets and resend later if
// we do not get an ack
//

static void SV_ReliableSend(nodeinfo_t *ni, netpacket_t *packet)
{
  netpacket_t sendpacket;

  // flag as reliable packet
  sendpacket.type = packet->type | pt_reliable;

  // copy data into sendpacket
  sendpacket.data.r.data = packet->data.u;
  
  // put packet number into packet
  sendpacket.data.r.packet_num = ni->packet_sent;

  // save packet data in nodeinfo_t
  ni->backup_packets[ni->packet_sent] = sendpacket;

  // save send time for timeout
  ni->backup_sendtime[ni->packet_sent] = I_GetTime_RealTime();

  ni->packet_sent = (ni->packet_sent + 1) % BACKUP_PACKETS;

  // send to client
  SendPacket(&ni->netnode, &sendpacket);
}

//==========================================================================
//
// SV_GamePacket
//
// Incoming Game Packets from clients are read and the tics inside them
// read out. We ignore tics we already have and take note of when we
// miss a tic so we can send a resend request to the client. Once we
// have all the ticcmds for a particular tic, we forward them to the
// clients.
//
//=========================================================================

//-------------------------------------------------------------------------
//
// SV_Lowtic
//
// Find the latest tic we can send to a player
//

int SV_Lowtic(int player)
{
  int i;
  int lowtic = -1;
  
  for(i=0; i<server_numplayers; i++)
    {
      int delta;
      
      if(!server_players[i].ingame)
	continue;

      if(player == i)           // send tic to player regardless of whether
	continue;               // we have received their ticcmd
      
      delta = server_players[i].got_tic - lowtic;
      
      if(lowtic == -1 ||
	 (delta < 0 && delta > -128) ||
	 (delta > 128))
	{
	  lowtic = server_players[i].got_tic;
	}  
    }

  return lowtic;
}

//--------------------------------------------------------------------------
//
// SV_SendSpeedupPackets
//
// Send packets to clients telling them to speedup/slowdown their
// tic sending.
//
// Based on the time we receive a particular tic from the clients
//

void SV_SendSpeedupPackets()
{
  netpacket_t packet;
  int i;
  int players = 0;
  int pivot_time = -1;
  
  for(i=0; i<server_numplayers; i++)
    players += (server_players[i].ingame);

  // if we have only 2 players, we distribute the lag between them
  // make sure they both send their packets at the same time -
  // this way they both get the same lag
  
  if(players <= 2)
    {
      for(i=0; i<server_numplayers; i++)
	{
	  playerinfo_t *pi = &server_players[i];
	  nodeinfo_t *ni = &server_nodes[pi->node];
	  int sendtime = pi->sync_time - (ni->latency/2);

	  if(pivot_time == -1)
	    pivot_time = sendtime;
	  else
	    {
	      int skiptics = pivot_time - sendtime;

	      if(skiptics)
		{
		  packet.type = pt_speedup;
		  packet.data.u.speedup.skiptics = skiptics;
		  
		  SV_ReliableSend(ni, &packet);
		}
	    }
	}
    }
  else

    // if we have more than two players, try to make all the tics arrive
    // at the server at the same time. This way, the lag experienced
    // should on average be equal to the ping time. This way, having
    // players with very high pings will not affect the lag of other
    // players with lower pings
    
    { 
      for(i=0; i<server_numplayers; i++)
	{
	  playerinfo_t *pi = &server_players[i];
	  nodeinfo_t *ni = &server_nodes[pi->node];

	  if(pivot_time == -1)
	    pivot_time = pi->sync_time;
	  else
	    {
	      int skiptics = pivot_time - pi->sync_time;

	      if(skiptics)
		{
		  packet.type = pt_speedup;
		  packet.data.u.speedup.skiptics = skiptics;
		  
		  SV_ReliableSend(ni, &packet);
		}
	    }
	}      
    }
}

//--------------------------------------------------------------------------
//
// SV_AddTic
//
// Deal with a tic_t read out of a gamepacket.
//

static void SV_AddTic(tic_t *tic)
{
  int player = tic->player;
  int ticnum = tic->ticnum;
  int expected_tic = (server_players[player].got_tic + 1) & 255;
  int i;

  // we want the next tic
  
  if(ticnum != expected_tic)
    {
      int delta = ticnum - expected_tic;

      if( (delta > 0 && delta < 128) ||
	  (delta < -128))
	{
	  // missed tic
	  // set resend
	  //	  C_Printf("server: missed tics %i (%i %i)\n", player, ticnum, expected_tic);
	  server_players[player].resend = true;
	}
      else
	{
	  // already got tic -- don't care
	  //	  if(player)
	  //	    C_Printf("server: dup tic %i (%i %i)\n", player, ticnum, expected_tic);
	}      

      return;
    }

  //  for(i=0; i<CONS_BYTES; i++)
  //    if(!tic->ticcmd.consdata[i])
  //      break;
  //    else
  //      {
  //	if(i == 0)
  //	  C_Printf("console data: ");
  //	C_Printf("%c",
  //		 isprint(tic->ticcmd.consdata[i]) ?
  //		 tic->ticcmd.consdata[i] : '*');
  //      }
  //  if(tic->ticcmd.consdata[0])
  //    C_Printf("\n");
  
  //  if(player)
  //  C_Printf("server: correct tic %i, %i\n", player, ticnum);
  
  // got correct tic
  // save tic data

  server_players[player].ticcmds[ticnum] = tic->ticcmd;
  server_players[player].got_tic = expected_tic;

  // sf: client speedup code
  // save the time when we receive player.ticcmds[0]
  
  if(ticnum == 0)
    {
      server_players[player].sync_time = I_GetTime_RealTime();

      // see if this is the last ticcmd for this tic

      if(SV_Lowtic(-1) == 0)
	{
	  // count down sync_countdown: when it reaches 0, send the
	  // speedup packets to the player

	  if(!sync_countdown--)
	    {
	      SV_SendSpeedupPackets();
	      sync_countdown = SYNC_COUNTDOWN_TIME;
	    }
	}
     }
  
  // no longer need resend
  server_players[player].resend = false;
  server_players[player].resend_time = -1;
  
  // see if this new ticcmd means we can send any new tics to clients
  
  for(i=0; i<server_numnodes; i++)
    {
      int lowtic;
      nodeinfo_t *ni = &server_nodes[i];

      if(ni->waiting) // ignore if player not yet in game
	continue;
      
      if(i == player) // ignore player tic came from
	continue;

      // find most recent tic that can be sent to player
      
      lowtic = SV_Lowtic(ni->player);

      // have we just got the last ticcmd the client needs for this tic?
      // if so, send it all the ticcmds
      
      if(lowtic == ticnum)
	{
	  int tic, p;
	  int starttic, endtic;
	  gamepacket_t gp;
	  
	  gp.num_tics = 0;

	  // include copies of previous ticcmds
	  
	  starttic = (ticnum - extratics) & 255;
	  endtic = (ticnum + 1) & 255;

	  // send ticcmds for all players
	  
	  for(p=0; p<server_numplayers; p++)
	    {
	      playerinfo_t *player = &server_players[p];
	      
	      if(!player->ingame)    // dont send if not in game
		continue;
	      if(ni->player == p)    // dont send tics to their origin
		continue;
	      
	      for(tic=starttic; tic!=endtic; tic = (tic+1) & 255)
		{
		  gp.tics[gp.num_tics].ticnum = tic;
		  gp.tics[gp.num_tics].player = p;
		  gp.tics[gp.num_tics].ticcmd = player->ticcmds[tic];
		  
		  gp.num_tics++;
		  
		  if(gp.num_tics >= NUM_TICS)
		    {
		      I_Error("server: gp.num_tics >= NUM_TICS");
		      SendGamePacket(&ni->netnode, &gp);
		      gp.num_tics = 0;
		    }
		}
	    }
	  
	  if(gp.num_tics)
	    SendGamePacket(&ni->netnode, &gp);	  
	}
    }  
}

//--------------------------------------------------------------------------
//
// SV_GamePacket
//
// Deal with a gamepacket received from a client.
//

static void SV_GamePacket(gamepacket_t *gp1)
{
  int i;
  int nodenum;
  gamepacket_t gp = *gp1;
  
  nodenum = SV_ServerNode();

  // only care about gamepackets from nodes _in_ the game
  
  if(nodenum == -1)
    {
      C_Printf("gamepacket from invalid node\n");
      return;
    }

  // add all tics in the packet
    
  for(i=0; i<gp.num_tics; i++)
    {
      SV_AddTic(&gp.tics[i]);
    }
}

//---------------------------------------------------------------------------
//
// SV_CheckResend
//
// Check if we need to send a resend request to one of the players
//

static void SV_CheckResendTics()
{
  int i;

  for(i=0; i<server_numplayers; i++)
    {
      playerinfo_t *player = &server_players[i];

      if(!player->ingame)
	continue;

      // check resend, time out if we have not received a response
      
      if(player->resend &&
	 I_GetTime_RealTime() >
            player->resend_time + server_nodes[player->node].latency)
	{
	  // need resend
	  // build resend request packet and send to player
      
	  netpacket_t packet;
	  clticresend_t *tr = &packet.data.u.clticresend;

	  // request all tics following the latest tic we have
	  
	  packet.type = pt_clticresend;
	  tr->ticnum = (player->got_tic + 1) & 255;
	  
	  SendPacket(&server_nodes[player->node].netnode, &packet);	

	  player->resend_time = I_GetTime_RealTime();
	}
    }
}

//=========================================================================
//
// Chat Relay
//
// We relay chat messages from clients to other nodes as well as informing
// them when other players join and quit
//
//=========================================================================

//-------------------------------------------------------------------------
//
// SV_SendChatMsg
//
// Send Chat message to nodes in chat room
//

static void SV_SendChatMsg(char *s, ...)
{
  char buffer[128];
  va_list args;
  int i;
  netpacket_t packet;
  
  va_start(args, s);
  vsprintf(buffer, s, args);
  va_end(args);

  // forward to waiting nodes

  for(i=0; i<server_numnodes; i++)
    {
      nodeinfo_t *ni = &server_nodes[i];

      if(!ni->waiting)       // only send to waiting players
	continue;

      // build new packet

      packet.type = pt_chat;
      strcpy(packet.data.u.messagepacket.message, buffer);

      // reliable send

      SV_ReliableSend(ni, &packet);
    }  
}

//-------------------------------------------------------------------------
//
// SV_SendPrivMsg
//
// Send private msg to a particular player
//

static void SV_SendPrivMsg(int node, char *s, ...)
{
  char buffer[128];
  va_list args;
  netpacket_t packet;
  nodeinfo_t *ni = &server_nodes[node];

  va_start(args, s);
  vsprintf(buffer, s, args);
  va_end(args);

  if(!ni->waiting)       // only send to waiting players
    return;
  
  // build new packet
  
  packet.type = pt_chat;
  strcpy(packet.data.u.messagepacket.message, buffer);

  // reliable send

  SV_ReliableSend(ni, &packet);
}

//-------------------------------------------------------------------------
//
// SV_ParsePrivateMsg
//
// Parse /msg commands
//

static void SV_ParsePrivateMsg(int src, char *cmd)
{
  char dest_player[20];
  int i;

  while(*cmd == ' ')  // strip leading spaces
    cmd++;

  // read out player name
  
  for(i=0; *cmd && *cmd != ' '; i++, cmd++)
    dest_player[i] = *cmd;
  dest_player[i] = '\0';

  for(i=0; i<server_numnodes; i++)
    {
      nodeinfo_t *ni = &server_nodes[i];
      
      if(!strcasecmp(dest_player, ni->name))
	{
	  if(!ni->waiting)
	    SV_SendPrivMsg(src, FC_GOLD"%s"FC_RED" is in game", dest_player);
	  else
	    {
	      SV_SendPrivMsg(i, FC_GRAY"[/msg]"FC_GOLD"%s"FC_RED"%s",
			     server_nodes[src].name,
			     cmd);
	      SV_SendPrivMsg(src, FC_GRAY">>"FC_GOLD"%s"FC_RED"%s",
			     dest_player,
			     cmd);
	    }

	  return;
	}
    }

  SV_SendPrivMsg(src, "unknown player %s", dest_player);
}

//-------------------------------------------------------------------------
//
// SV_ChatMsg
//
// Forward chat messages between waiting nodes
//

static void SV_ChatMsg(msgpacket_t *msg)
{
  int node;
  
  node = SV_ServerNode();

  if(node < 0)
    return;
  
  // action
  
  if(!strncasecmp(msg->message, "/me", 3))
    SV_SendChatMsg(FC_GOLD "%s" FC_RED " %s",
		   server_nodes[node].name, msg->message+3);
  else if(!strncasecmp(msg->message, "/msg", 4))
    SV_ParsePrivateMsg(node, msg->message+4);
  else
    SV_SendChatMsg(FC_GRAY "<" FC_GOLD "%s" FC_GRAY ">" FC_RED " %s",
		   server_nodes[node].name,
		   msg->message);		 
}

//=========================================================================
//
// Ping
//
// It is important to know the latency of the connected clients for
// several reasons: eg. we need to know how long to wait for a
// timeout before resending a packet.
//
//=========================================================================

static int ping_sendtime;

//-------------------------------------------------------------------------
//
// SV_CheckPing
//
// Resend ping requests every ~20 seconds
//

static void SV_CheckPing()
{
  if(I_GetTime_RealTime() > ping_sendtime + PING_FREQ*TICRATE)
    {
      int i;
      netpacket_t packet;
      
      ping_sendtime = I_GetTime_RealTime();
      
      packet.type = pt_ping;
      
      for(i=0; i<server_numnodes; i++)
	SendPacket(&server_nodes[i].netnode, &packet);
    }
}

//-------------------------------------------------------------------------
//
// SV_Pong
//
// Called when ping response receieved
//

static void SV_Pong()
{
  int node;

  node = SV_ServerNode();
  if(node == -1)
    return;      // how?
  
  server_nodes[node].latency = I_GetTime_RealTime() - ping_sendtime;
}

//-------------------------------------------------------------------------
//
// SV_SendPingReply
//
// Send a pong packet to someone who is pinging us
//

static void SV_Ping()
{
  netpacket_t packet;

  packet.type = pt_pong;

  SendPacket(&source, &packet);
}

//=========================================================================
//
// Other Types of Packet
//
// As well as game packets containing client ticcmds, we can also
// receive other types of packet: to join or quit the server, finger
// it etc.
//
//=========================================================================

//-------------------------------------------------------------------------
//
// SV_FingerRequest
//
// Send reply to finger request
//

static void SV_FingerRequest()
{
  netpacket_t packet;
  fingerpacket_t *fp = &packet.data.u.fingerpacket;
  int i;

  //  C_Printf("finger request from %s:%i\n", source.netmodule->name, source.node);
  
  packet.type = pt_finger;
  
  // copy server name
  strcpy(fp->server_name, server_name);

  // number of players in game
  fp->players = 0;
  for(i=0; i<server_numplayers; i++)
    if(server_players[i].ingame)
      fp->players++;
  
  fp->players = server_numnodes;
  fp->accepting = waiting_players;
  strncpy(fp->server_os, version_os, 6);

  // etc.

  // send packet to node

  SendPacket(&source, &packet);
}

//-------------------------------------------------------------------------
//
// SV_NodeQuit
//
// Called when a remote node disconnects
//

static void SV_NodeQuit(quitpacket_t *qp)
{
  int nodenum = SV_ServerNode();
  int i;
  
  if(nodenum == -1)
    {
      // not in game anyway
      return;
    }

  // inform people in chat room

  SV_SendChatMsg("%s quit (%s)",
		 server_nodes[nodenum].name,
		 qp->quitmsg);
    
  // remove from node list

  for(i=nodenum; i<server_numnodes-1; i++)
    {
      server_nodes[i] = server_nodes[i+1];
      if(server_nodes[i].player != -1)
	server_players[server_nodes[i].player].node = i;
    }

  server_numnodes--;

  // recalculate controller node

  SV_FindController();
  
  // if node was not actually in the game yet, or a drone, do not
  // forward quit to players

  if(!server_nodes[nodenum].waiting && !server_nodes[nodenum].drone)
    {
      server_players[(int)qp->player].ingame = false;
      
      // forward quit to remaining nodes
      
      for(i=0; i<server_numnodes; i++)
	{
	  netpacket_t packet;
	  nodeinfo_t *ni = &server_nodes[i];

	  if(ni->waiting)
	    continue;
	  
	  // build packet
	  
	  packet.type = pt_quit;
	  packet.data.u.quitpacket = *qp;
	  
	  // send packet
	  SendPacket(&ni->netnode, &packet);
	}
    }

  // send new wait info
  
  SV_SendWaitInfo();
}

//-------------------------------------------------------------------------
//
// SV_TicResendRequest
//
// Resend Tics to client which has missed tics
//

static void SV_TicResendRequest(svticresend_t *rp)
{
  gamepacket_t gp;
  int i, n;
  int lowtic = -1;
  int stoptic;
  int node;

  //  C_Printf("server: resend request\n");
  
  node = SV_ServerNode();
  if(node == -1)
    return;
  
  // find lowtic
  
  lowtic = SV_Lowtic(server_nodes[node].player);
  stoptic = (lowtic + 1) & 255;
  
  // build resend packet and send to client
  
  gp.num_tics = 0;

  //  C_Printf("server: resend ");
  
  for(i=0; i<server_numplayers; i++)
    {
      playerinfo_t *player = &server_players[i];
      
      if(!player->ingame)
	continue;
      if(player->node == node)      // dont send ticcmds to their source
	continue;      

      // add all missed tics for this player
      // do not send ticcmds for tics we do not yet have
      
      for(n=rp->ticnum[i]; n!=stoptic; n = (n+1) & 255)
        {
	  //C_Printf("%i ", n);
	  gp.tics[gp.num_tics].player = i;
	  gp.tics[gp.num_tics].ticnum = n;
	  gp.tics[gp.num_tics].ticcmd = player->ticcmds[n];

	  gp.num_tics++;

	  // packet full?
	  
	  if(gp.num_tics >= NUM_TICS)
	    {
	      SendGamePacket(&source, &gp);
	      gp.num_tics = 0;
	    }
	}
    }

  // send any remaining tics

  if(gp.num_tics)
    SendGamePacket(&source, &gp);

  //  C_Printf("\n");
}

//=========================================================================
//
// Server Joining
//
// When the server is started, we turn on waiting_players to allow new
// nodes to connect. The first node to connect becomes the controller.
// Once enough nodes have joined, the controller sends a packet to the
// server calling to start the game. The server forwards this to the
// clients to start the game.
//
//=========================================================================

//-------------------------------------------------------------------------
//
// SV_StartGameSignal
//
// When all players have joined, the controller sends a
// packet with type pt_startgame to the server. The server
// then sends another packet to all of the connected
// nodes.

static void SV_StartGameSignal(startgame_t *sg)
{
  netpacket_t packet;
  int i;
  int ticdup;
  long rndseed = time(NULL);

  // if correctpacket returned true then it must be from a connected
  // node. now we make sure this is coming from the controller

  i = SV_ServerNode();
  
  if(i != controller)
    return;
  
  // get ticdup from packet

  ticdup = sg->ticdup;

  // send start game signal to client nodes
  
  sg = &packet.data.u.startgame;

  //  C_Printf("start game:%i\n", waiting_players);
  
  //  if(!waiting_players)
  //    return;

  // allocate player numbers to all the client nodes first
  
  server_numplayers = 0;

  for(i=0; i<server_numnodes; i++)
    {
      nodeinfo_t *ni = &server_nodes[i];
      
      ni->waiting = false;
      
      if(ni->drone)
	ni->player = -1;
      else
	{
	  ni->player = server_numplayers++;
	  
	  server_players[ni->player].node = i;
	  server_players[ni->player].ingame = true;
	  server_players[ni->player].got_tic = 0;
	  server_players[ni->player].resend = false;
	}
    }

  // send start game signals to clients
  
  for(i=0; i<server_numnodes; i++)
    {
      nodeinfo_t *ni = &server_nodes[i];

      // send startgame packet
  
      packet.type = pt_startgame;

      // include various data

      sg->player = ni->player;
      sg->num_players = server_numplayers;

      sg->rndseed[0] = rndseed & 255;
      sg->rndseed[1] = (rndseed >> 8) & 255;
      sg->rndseed[2] = (rndseed >> 16) & 255;
      sg->rndseed[3] = (rndseed >> 24) & 255;

      sg->ticdup = ticdup;
      
      // reliable send data
      
      SV_ReliableSend(ni, &packet);
    }

  // stop accepting joins now
  waiting_players = false;
}

//-------------------------------------------------------------------------
//
// SV_SendWaitInfo
//
// Send info to players while waiting for the server to start:
// number of players and their names
//

static void SV_SendWaitInfo()
{
  netpacket_t packet;
  waitinfo_t *wi;
  int i;

  // build packet
  
  packet.type = pt_waitinfo;
  wi = &packet.data.u.waitinfo;

  wi->nodes = 0;

  for(i=0; i<server_numnodes; i++)
    {
      // build name:
      // translucent if already in game (unable to chat)
      // add a white '@' if the controller

      sprintf(wi->node_names[wi->nodes++],
	      "%s%s%s", 
	      server_nodes[i].waiting ? "" : FC_TRANS,
	      controller == i ? FC_GRAY "@" FC_RED : "",
	      server_nodes[i].name);
    }
  
  // send to all waiting nodes

  for(i=0; i<server_numnodes; i++)
    {
      nodeinfo_t *ni = &server_nodes[i];
      
      if(ni->waiting)
	{
	  // tell node if they are the controller

	  wi->controller = i == controller;

	  // reliable send

	  SV_ReliableSend(ni, &packet);
	}
    }
}

//---------------------------------------------------------------------------
//
// SV_DenyJoin
//
// Used by SV_JoinRequest to send a connecting client a response
// denying their connection.
//

static void SV_DenyJoin(char *reason)
{
  netpacket_t packet;
  
  // send back a deny packet
  packet.type = pt_deny;
  strcpy(packet.data.u.denypacket.reason, reason);
  
  // send
  
  SendPacket(&source, &packet);

  return;
}

//---------------------------------------------------------------------------
//
// SV_AcceptJoin
//
// Used by SV_JoinRequest to send a connecting client a response
// accepting their connection.
//

static void SV_AcceptJoin(joinpacket_t *jp)
{
  netpacket_t packet;
  nodeinfo_t *ni;
  
  //  C_Printf("accepted join from %s\n", jp->name);

  // add node to list

  ni = &server_nodes[server_numnodes++];
  
  ni->waiting = true;
  ni->netnode = source;
  strcpy(ni->name, jp->name);

  // clear packet send data

  ni->packet_sent = ni->packet_acked = 0;
  ni->packet_expected = 0;

  // find controlling node
  
  SV_FindController();
  
  // send accept packet to joining node
  
  packet.type = pt_accept;

  // include server name
  strcpy(packet.data.u.acceptpacket.server_name, server_name);

  // must be reliable

  SV_ReliableSend(ni, &packet);

  // send updated wait info to waiting client nodes
  
  SV_SendWaitInfo();

  // get ping from new client

  ping_sendtime = -PING_FREQ * TICRATE;

  // inform people in chat room

  SV_SendChatMsg("%s joined server", jp->name);
}

//-------------------------------------------------------------------------
//
// SV_JoinRequest
//
// Clients send this when they want to connect to the server
//

static void SV_JoinRequest(joinpacket_t *jp)
{  
  unsigned long wadsig;
  
  // different doom versions cannot play a netgame!
  
  if(jp->version != VERSION)
    {
      SV_DenyJoin("incorrect game version!");
      return;
    }

  if(server_numnodes >= MAXNODES)
    {
      SV_DenyJoin("server is full");
      return;
    }
  
  // check player is using the right wad
  // if this is the first node, copy the signature

  wadsig =
    jp->wadsig[0] +
    (jp->wadsig[1] << 8) +
    (jp->wadsig[2] << 16) +
    (jp->wadsig[3] << 24);

  if(server_numnodes > 0)
    {
      //      if(wad_signature != wadsig)
      //	{
      //	  SV_DenyJoin("wrong wads loaded");
      //	  return;
      //	}
    }
  else
    wad_signature = wadsig;
  
  
  // if not currently accepting connections, send back deny

  //  if(!waiting_players)
  //    {
  //      SV_DenyJoin("not currently accepting joins");
  //      return;
  //    }

  
  if(SV_ServerNode() != -1)
    {
      // already in game
      // perhaps they missed the joining packet, send them
      // another one    

      // this will be done by the reliable packet send
      // timing out anyway

      return;
    }

  // accept join

  SV_AcceptJoin(jp);
}

//=========================================================================
//
// Packet Listening
//
// We can set up multiple netmodules to listen on for incoming packets.
// New Packets are dealt with by SV_NetPacket which calls other functions
// depending on the packet type.
//
//=========================================================================

//-------------------------------------------------------------------------
//
// SV_NetPacket
//
// Deal with an incoming netgame packet
//

static void SV_NetPacket(netpacket_t *packet)
{
  int type = packet->type;
  union packet_data *data;

  //  C_Printf("packet from %s:%i\n",
  //    	   source.netmodule==&loopback_server?"loopback":"other",
  //    	   source.node);

  // reliable packet - need to ack packet
  
  if(type & pt_reliable)
    {
      type &= ~pt_reliable;
      data = &packet->data.r.data;

      // check packet is in the correct order
      // send ack reply to client

      if(!SV_CorrectPacket(packet->data.r.packet_num))
	return;
    }
  else
    data = &packet->data.u;
  
  switch(type)
    {
      case pt_gametics:
	SV_GamePacket(&data->gamepacket);
	break;

      case pt_compressed:                // compressed gametics
	SV_GamePacket
	  (DecompressPacket(&data->compressed));
	break;
	
      case pt_console:
	//	SV_ConsolePacket
	// forward console packet to all other nodes
	break;
	
      case pt_chat:
	SV_ChatMsg(&data->messagepacket);
	break;
	
      case pt_fingerrequest:
	// fingering server: send response
	SV_FingerRequest();
	break;
	
      case pt_join:
	SV_JoinRequest(&data->joinpacket);
	break;

      case pt_ack:
	SV_AckPacket(&data->ackpacket);
	break;

      case pt_startgame:
	SV_StartGameSignal(&data->startgame);
	break;

      case pt_quit:         // remote node disconnecting
	SV_NodeQuit(&data->quitpacket);
	break;

      case pt_svticresend:
	SV_TicResendRequest(&data->svticresend);
	break;

      case pt_ping:
	SV_Ping();
	break;
	
      case pt_pong:
	SV_Pong();
	break;
	
      default:
	C_Printf("server: unknown packet type %i\n"
		 "from %s:%i\n",
		 packet->type,
		 source.netmodule->name,
		 source.node);
	break;
    }
}


//-------------------------------------------------------------------------
//
// SV_GetPackets
//
// Check all enabled net modules, and get any waiting packets
//

netmodule_t *server_netmodules[MAXNETMODULES];

// SV_GetPackets

static void SV_GetPackets()
{
  netpacket_t *incoming;
  int i;
  
  for(i = 0; server_netmodules[i]; i++)
    {
      // get all incoming packets from this module

      source.netmodule = server_netmodules[i];
      
      while(1)
	{
	  incoming =
	    (netpacket_t *)server_netmodules[i]->GetPacket(&source.node);

	  if(!incoming)
	    break;

	  SV_NetPacket(incoming);
	}
    }
}

//==========================================================================
//
// SV_Update
//
// The Server's main function: this is called by NetUpdate occasionally
// to check for new packets sent to the server.
//
//==========================================================================

void SV_Update()
{
  if(!server_active)
    return;

  SV_GetPackets();               // get new packets
  SV_CheckResend();              // check if we need to resend packets
  SV_CheckResendTics();          // need resend if we have missed tics
  SV_CheckPing();                // ping clients occasionally
}

//===========================================================================
//
// Init Server
//
// Called at startup
//
//===========================================================================

void SV_Init()
{
  server_active = false;
}

//===========================================================================
//
// Server Setup
// Startup/Shutdown Server
//
//===========================================================================

//---------------------------------------------------------------------------
//
// SV_StartServer
//
// Start up a server
// Note: this will not set any server netmodules:
//       ie. use SV_AddModule to add some server modules
//

void SV_ClearModules()
{
  server_netmodules[0] = NULL;    // start with end -- no modules
}

//
// SV_AddModule
// returns true if successfully added
//

boolean SV_AddModule(netmodule_t *netmodule)
{
  int i;

#ifdef DEDICATED

  if(!netmodule->Init())
    return false;

#else /* we need more checks in dedicated server */

  if(netmodule == &loopback_client)
    netmodule = &loopback_server;

  // initialise module
  
  if(!netmodule->Init())
    {
      MN_ErrorMsg("error initialising %s", netmodule->name);
      return false;
    }

#endif /* #ifdef DEDICATED */
  
#ifdef DJGPP
  
  // set up waiting for call
  if(netmodule == &modem)
    Ser_WaitForCall();
    
#endif

  // find end of list
  
  for(i=0; server_netmodules[i]; i++);

  // add to end of lsit
  
  server_netmodules[i++] = netmodule;
  server_netmodules[i] = NULL;

  return true;
}

void SV_StartServer()
{
  char tempstr[64];

  // ** init server name **
  
#ifdef DEDICATED
  // name server after the hostname in dedicated server
  {
    char hostname[64];
    gethostname(hostname, 64);
    sprintf(tempstr, "server @ %s", hostname);
  }
#else
  sprintf(tempstr, "%s's server", default_name);
#endif
  server_name = strdup(tempstr);

  // clear everything
  SV_ClearModules();

  // no players
  server_numnodes = 0;
  server_numplayers = 0;
  
  // active server, awaiting players to join
  server_active = true;
  waiting_players = true;  
}

//---------------------------------------------------------------------------
//
// SV_Shutdown
//
// Shutdown server
//

void SV_Shutdown()
{
  int i, n;

  if(!server_active)
    return;
  
  // inform connected nodes

  for(i=0; i<server_numnodes; i++)
    {
      netpacket_t packet;

      packet.type = pt_shutdown;

      for(n=0; n<3; n++)
	SendPacket(&server_nodes[i].netnode, &packet);
    }

  //  delay(300);
  
  server_numnodes = 0;
  server_active = false;

  // shutdown net modules

  for(i=0; server_netmodules[i]; i++)
    {
      server_netmodules[i]->Shutdown();
    }

  C_Printf(FC_GRAY "server shut down\n");
}


#ifndef DEDICATED /* no console cmds in dedicated server */

//--------------------------------------------------------------------------
//
// Console Commands
//

CONSOLE_COMMAND(server, cf_buffered)
{
  int i;
  
  SV_StartServer();

  // allow loopback local connections
  
  SV_AddModule(&loopback_server);

  // add modules from cmd line
  
  for(i=0; i<c_argc; i++)
    {
      netmodule_t *nm;

      nm = Net_ModuleForName(c_argv[i]);
      if(nm)
	{
	  if(!SV_AddModule(nm))
	    return;
	}
      else
	C_Printf("unknown module \"%s\"\n", c_argv[i]);
    }

  // connect local client to server

  CL_LoopbackConnect();
}

CONSOLE_COMMAND(sv_latency, 0)
{
  int i;
  
  if(!server_active)
    return;

  C_Printf("latency from clients:\n");

  for(i=0; i<server_numnodes; i++)
    C_Printf("latency from %s: %i\n",
	     server_nodes[i].name,
	     server_nodes[i].latency);
}

CONSOLE_STRING(sv_name, server_name, NULL, 25, cf_nosave) {}

void SV_AddCommands()
{
  C_AddCommand(server);
  C_AddCommand(sv_latency);
  C_AddCommand(sv_name);
}

#endif /* #ifdef DEDICATED */

//---------------------------------------------------------------------------
//
// $Log$
// Revision 1.15  2000-08-16 13:29:14  fraggle
// more generalised os detection
//
// Revision 1.14  2000/06/22 18:25:44  fraggle
// os_t -> doomos_t to peacefully co-exist with allegro
//
// Revision 1.13  2000/06/20 21:08:35  fraggle
// platform detection (dos, win32, linux etc)
//
// Revision 1.12  2000/06/19 14:58:55  fraggle
// cygwin (win32) support
//
// Revision 1.11  2000/06/04 17:19:02  fraggle
// easier reliable-packet send interface
//
// Revision 1.10  2000/05/24 13:36:05  fraggle
// secure up server a bit
//
// Revision 1.9  2000/05/22 09:59:04  fraggle
// /msg, /me commands for chat room
//
// Revision 1.8  2000/05/12 16:41:59  fraggle
// even better speeddup algorithm
//
// Revision 1.7  2000/05/07 14:10:28  fraggle
// better time adjustment algorithm
//
// Revision 1.6  2000/05/07 13:11:21  fraggle
// improve multiplayer chatroom interface
//
// Revision 1.5  2000/05/06 14:06:11  fraggle
// fix ticdup
//
// Revision 1.4  2000/05/03 16:46:45  fraggle
// check wads in netgames
//
// Revision 1.3  2000/05/03 16:21:23  fraggle
// client speedup code
//
// Revision 1.2  2000/05/02 15:43:41  fraggle
// client movement prediction
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//---------------------------------------------------------------------------

