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

/*****************************

  Large comments like this
  are where I have not yet
  finished writing some code
  
*****************************/

#include "z_zone.h"

#include "am_map.h"
#include "c_io.h"
#include "c_net.h"
#include "doomdef.h"
#include "doomstat.h"
#include "d_deh.h"
#include "d_main.h"
#include "f_wipe.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_system.h"
#include "m_random.h"
#include "mn_engin.h"
#include "p_user.h"
#include "r_draw.h"
#include "s_sound.h"

#include "d_player.h"
#include "net_gen.h"
#include "net_modl.h"
#include "sv_serv.h"

void D_ProcessEvents();
void G_BuildTiccmd (ticcmd_t *cmd); 
void D_DoAdvanceDemo (void);

extern boolean advancedemo;

// many variables.
// clean me

void ResetNet();

#define CONSISTANCY_BUFFER 32
static byte consistancy[CONSISTANCY_BUFFER];

int prediction_threshold = 16;    // max. number of tics to predict 16 = full

netnode_t server;               // the server's netnode_t for comms

boolean isconsoletic;
boolean drone;
extern int consoleplayer;

static int pending_ticdup = 1;      // new ticdup we are waiting to set
int ticdup = 1;
int extratics = 2;

extern int levelstarttic, basetic;

extern boolean netgame;

player_t players[MAXPLAYERS];

int gametic;                    // current game time
//int maketic;                  // the tic we are currently making
#define maketic (nettics[consoleplayer])

ticcmd_t backup_tics[MAXPLAYERS][256];
ticcmd_t player_cmds[MAXPLAYERS];

static boolean tic_resend;    // set if we need to send a resend req to server
static int tic_resendtime;    // time we sent the resend request - eventually
                              // time out and send another resend request

int nettics[MAXPLAYERS];
boolean playeringame[MAXPLAYERS];

// server ping time

static int latency;

// 'open socket' checking
// last time we received a packet from the server

boolean opensocket;
static int lastpacket_time;

boolean out_of_sync;

static int gametime;

int cl_stack=2;

//===========================================================================
//
// Reliable Packet Send
//
// Certain packets (console cmds, player quit etc) need to be sent
// "reliably" - ie. we need to know they have arrived. CL_CorrectPacket
// checks that an incoming packet from a client is in the correct order.
// We do not use this kind of send for game (tic) packets as it consumes
// too much bandwidth
//
// Currently checks that packets are in the correct order.
// It would be better to change this to a timeout/ack based system.
//
//=========================================================================

// sent 

static netpacket_t backup_packets[BACKUP_PACKETS];
static int backup_sendtime[BACKUP_PACKETS];
static int packet_sent, packet_acked;

// received

static int packet_expected;

//--------------------------------------------------------------------------
//
// CL_CheckResend
//
// If we have not received an ack after a certain time, resend packet
//

static void CL_CheckResend()
{
  int p;

  for(p=packet_acked; p!=packet_sent; p = (p+1) % BACKUP_PACKETS)
    {
      if(I_GetTime_RealTime() > backup_sendtime[p] + latency)
	{
	  // timeout: resend packet
	  SendPacket(&server, &backup_packets[p]);
	  backup_sendtime[p] = I_GetTime_RealTime();
	  //	  C_Printf("cl: resending %i\n", p);
	}
    }
}

//-------------------------------------------------------------------------
//
// CL_AckPacket
//
// When we get an acknowledgement reply from the server
//

static void CL_AckPacket(ackpacket_t *ack)
{
  int delta = ack->packet - packet_acked;
  
  // already got ack
  
  if(delta < 0 && delta > -64)
    return;

  packet_acked = (ack->packet + 1) % BACKUP_PACKETS;

  //  C_Printf("cl: acked up to %i\n", packet_acked);
}

//-------------------------------------------------------------------------
//
// CL_CorrectPacket
//
// Check the reliable packet we have received from the server is the
// correct one
//

static boolean CL_CorrectPacket(int packet_num)
{
  int delta = packet_num - packet_expected;

  //  C_Printf("cl: delta %i (%i - %i)\n", delta, packet_num, packet_expected);
  
  // correct packet ?
  if(delta == 0)
    packet_expected = (packet_num + 1) % BACKUP_PACKETS;
  
  // send acknowledgement if it is for a packet we want or already have

  if(delta <= 0 && delta > -64)
    {
      netpacket_t reply;

      reply.type = pt_ack;
      reply.data.ackpacket.packet = (packet_expected - 1) % BACKUP_PACKETS;

      // C_Printf("cl: send ack to %i\n", reply.data.ackpacket.packet);
      
      SendPacket(&server, &reply);
    }

  return delta == 0;
}

//-------------------------------------------------------------------------
//
// CL_ReliableSend
//
// Send packet to server, but store a backup so we can send again if
// it is not acked
//

static void CL_ReliableSend(netpacket_t *packet)
{
  // save packet data in backup_packets
  packet->data.packet_num = packet_sent;
  backup_packets[packet_sent] = *packet;

  // save send time
  backup_sendtime[packet_sent] = I_GetTime_RealTime();

  packet_sent = (packet_sent + 1) % BACKUP_PACKETS;

  // send to server
  SendPacket(&server, packet);
}

//=========================================================================
//
// Server Connect/Disconnect
//
//=========================================================================

//-------------------------------------------------------------------------
//
// CL_Disconnect
//
// Disconnect from the server
//

void CL_Disconnect(char *reason)
{
  // shut down client
  
  if(netgame && !demoplayback)
    {  
      netpacket_t packet;
      quitpacket_t *qp = &packet.data.quitpacket;
      int i;

      packet.type = pt_quit;
      
      qp->player = drone ? -1 : consoleplayer;
      strcpy(qp->quitmsg, reason);

      // send several packets to make sure that the server
      // receives the packet
      
      for(i=0; i<5; i++)
	SendPacket(&server, &packet);
      
      // disconnect from server
      
      server.netmodule->Shutdown();
      
      netgame = false;
      
      consoleplayer = 0;

      ticdup = 1;
      
      //deathmatch = 0;
      for(i=0;i<MAXPLAYERS;i++)
	{
	  playeringame[i] = i == consoleplayer;
	}

      gametime = I_GetTime();
      
      C_SetConsole();
      C_Printf(FC_GRAY "client disconnected\n");
    }
}

//--------------------------------------------------------------------------
//
// NetDisconnect
//
// Higher-level shutdown
// Shutdown server and client
//

void NetDisconnect(char *reason)
{
  // shut down client
  CL_Disconnect(reason);

  // shut down server

  SV_Shutdown();

  // drop to console
  
  C_SetConsole();
}

//-------------------------------------------------------------------------
//
// CL_Connect
//
// Connect to a server awaiting connections
//

extern gamestate_t wipegamestate;
extern gamestate_t oldgamestate;

static char server_name[50];
static boolean got_waitinfo;
static waitinfo_t waitinfo;

#define CHAT_LINES 14
#define CHAT_MAXINPUT 40
static char *chat_msgs[CHAT_LINES];
static char chat_input[CHAT_MAXINPUT];

// helper functions:

// CL_SendJoin: send a join packet to the server

static void CL_SendJoin(netnode_t *netnode)
{
  netpacket_t packet;
  unsigned long wadsig;
  
  // display "connecting" box in menu
  
  if(menuactive)
    V_SetLoading(0, "trying...");
  else
    usermsg("trying...");

  packet.type = pt_join;
  packet.data.joinpacket.drone = 0;          // not a drone
  packet.data.joinpacket.version = VERSION;
  strcpy(packet.data.joinpacket.name, default_name);

  wadsig = W_Signature();
  packet.data.joinpacket.wadsig[0] = wadsig & 255;
  packet.data.joinpacket.wadsig[1] = (wadsig >> 8) & 255;
  packet.data.joinpacket.wadsig[2] = (wadsig >> 16) & 255;
  packet.data.joinpacket.wadsig[3] = (wadsig >> 24) & 255;
  
  SendPacket(netnode, &packet);
}

// CL_SendStartGame: Send Packet to the server to start game

static void CL_SendStartGame()
{
  netpacket_t packet;
  
  packet.type = pt_startgame;
  packet.data.startgame.ticdup = pending_ticdup; // use new ticdup
  
  // reliable send
  
  CL_ReliableSend(&packet);
}

// CL_ChatPrint: add line to chat messages

static void CL_ChatPrint(char *s)
{
  int i;

  if(chat_msgs[0])
    Z_Free(chat_msgs[0]);

  for(i=0; i<CHAT_LINES-1; i++)
    chat_msgs[i] = chat_msgs[i+1];

  chat_msgs[CHAT_LINES-1] = Z_Strdup(s, PU_STATIC, 0);
}

// CL_SendChatMsg: send chat message to server

static void CL_SendChatMsg(char *s)
{
  netpacket_t packet;
  msgpacket_t *msg = &packet.data.messagepacket;

  packet.type = pt_chat;

  // put message in packet

  strcpy(msg->message, s);

  // reliable send

  CL_ReliableSend(&packet);
}

// CL_Connect: connect to a particular server

void CL_Connect(netnode_t *netnode)
{
  int connection_attempts = 0;
  netpacket_t *packet;
  int source_node;
  boolean got_connection = false;
  int i;

  c_showprompt = false;
  
  CL_Disconnect("changing server");                // disconnect first

  // initialise module used
  
  if(!netnode->netmodule->Init())
    {
      // module init error
      C_Printf("failed to init %s module\n", netnode->netmodule->name);
      return;
    }

#ifdef DJGPP
  // dial computer if we have to

  if(netnode->netmodule == &modem)
    Ser_Dial();

#endif

  server = *netnode;

  // expect first packet
  
  packet_sent = packet_acked = packet_expected = 0;
  
  do
    {
      int retrytime;
      
      CL_SendJoin(netnode);
      connection_attempts++;
      
      retrytime = I_GetTime_RealTime() + 35;
      
      while(I_GetTime_RealTime() < retrytime)
	{
	  SV_Update();

	  // read new packets
	  packet = netnode->netmodule->GetPacket(&source_node);

	  // nothing received yet?
	  if(!packet)
	    continue;
	  
	  // ignore packets not from server
	  if(source_node != netnode->node)
	    {
	      //  C_Printf("CL_Connect: packet not from server\n");
	      continue;
	    }

	  if(packet->type == pt_accept)
	    { 
	      usermsg("connection accepted!");

	      strcpy(server_name, packet->data.acceptpacket.server_name);
	      usermsg("\n%s\n", server_name);
	      //	      controller = packet->data.acceptpacket.controller != 0;
	      got_connection = true;

	      // send reply to server (ack packet)
	      CL_CorrectPacket(packet->data.acceptpacket.packet_num);
	      
	      break;
	    }

	  if(packet->type == pt_deny)
	    {
	      C_Printf("connection denied\n");
	      C_Puts(packet->data.denypacket.reason);
	      MN_ErrorMsg("connection denied:\n%s",
			  packet->data.denypacket.reason);
	      netnode->netmodule->Shutdown();
	      return;
	    }
	}

      if(got_connection)
	break;
      
    } while(connection_attempts < 5);

  if(!got_connection)
    {
      if(menuactive)
	MN_ErrorMsg("connect failed.");
      else
	C_Printf("connect failed.\n");
      netnode->netmodule->Shutdown();
      return;
    }
  
  // wait for start game signal

  usermsg("waiting for start game signal");

  // put into wait for start game gamestate
  
  got_waitinfo = false;
  wipegamestate = oldgamestate = gamestate = GS_SERVERWAIT;
  C_InstaPopup();
  menuactive = false;
  
  netgame = true;

  // muzak while-u-wait

  S_StartRandomMusic();

  // clear chat data

  for(i=0; i<CHAT_LINES; i++)
    {
      if(chat_msgs[i])
	Z_Free(chat_msgs[i]);
      chat_msgs[i] = NULL;
    }

  chat_input[0] = '\0';

  CL_ChatPrint(FC_GRAY "connected to server.");
}

//-------------------------------------------------------------------------
//
// CL_LoopbackConnect
//
// Connect to local server
//

void CL_LoopbackConnect()
{
  netnode_t localnode;

  localnode.netmodule = &loopback_client;
  localnode.node = 0;
  
  CL_Connect(&localnode);
}

//=========================================================================
//
// While we are waiting for the server to start the game, we enter
// into the GS_WAITSERVER gamestate. We draw player info received
// from the server
//
//=========================================================================

//-------------------------------------------------------------------------
//
// CL_FindController
//
// Find which player is the game controller
//

int controller;

static void CL_FindController()
{
  int i;

  for(i=0; i<MAXPLAYERS; i++)
      if(playeringame[i])
	break;

  controller = i == MAXPLAYERS ? -1 : i;  
}

//-------------------------------------------------------------------------
//
// CL_WaitDrawer
//
// Draw wait info while we wait for the server
//

#define CLIENT_INFO                 \
    "ctrl-d to disconnect\n"        \
    "ctrl-m to change muzak\n"
#define SERVER_INFO                 \
    "ctrl-enter to start game"
#define SEPERATOR "{||||||||||||||||||||||||||||}"
#define WAITING                     \
    "connected to server\n"         \
    "waiting to receive\n"          \
    "data from server"

void CL_WaitDrawer()
{
  char *disc_message;
  int i;
  int basey, y;
  char tempstr[CHAT_MAXINPUT + 5];

  V_DrawBackground("FLAT5", screens[0]);

  y = 4;

  // write server name
  
  V_WriteText(server_name,
	      (SCREENWIDTH - V_StringWidth(server_name)) / 2,
	      y);
  y += V_StringHeight(server_name) + 5;

  // 'ctrl-d to disconnect' etc

  disc_message = got_waitinfo && waitinfo.controller ?
    CLIENT_INFO
    SERVER_INFO
    :
    CLIENT_INFO;

  V_WriteText(disc_message,
	      (SCREENWIDTH - V_StringWidth(disc_message)) / 2,
	      y);
  y += V_StringHeight(disc_message) + 2;

  // draw seperator

  V_WriteText(SEPERATOR,
	      (SCREENWIDTH - V_StringWidth(SEPERATOR)) / 2,
	      y);

  y += V_StringHeight(SEPERATOR) + 4;

  basey = y;

  // if we do not have waitinfo yet, display a waiting box

  if(!got_waitinfo)
    {
      int wid = V_StringWidth(WAITING), height = V_StringHeight(WAITING);
      V_DrawBox((SCREENWIDTH-wid)/2 - 4, (SCREENHEIGHT-height)/2 - 4,
		wid + 8, height + 8);
      V_WriteText(WAITING, (SCREENWIDTH-wid) / 2, (SCREENHEIGHT-height)/2);
      return;
    }

  // draw chat data

  for(i=0; i<CHAT_LINES; i++)
    if(chat_msgs[i])
      {
	V_WriteText(chat_msgs[i], 10, y);
	y += V_StringHeight(chat_msgs[i]);
      }
    else
      y += 8;

  // draw chat prompt

  sprintf(tempstr, FC_GOLD ">" FC_RED "%s_", chat_input);
  V_WriteText(tempstr, 10, y);

  // draw list of players on server

  if(got_waitinfo)
    {
      int width = 0, height = 0;;

      // find box size

      for(i=0; i<waitinfo.nodes; i++)
	{
	  height += V_StringHeight(waitinfo.node_names[i]);
	  if(V_StringWidth(waitinfo.node_names[i]) > width)
	    width = V_StringWidth(waitinfo.node_names[i]);
	}

      // draw box
      
      V_DrawBox(SCREENWIDTH - width - 8, basey - 4,
		width+4, height+4);

      // draw player names

      y = basey;

      for(i=0; i<waitinfo.nodes; i++)
	{
	  V_WriteText(waitinfo.node_names[i], 
		      SCREENWIDTH - width - 4,
		      y);
	  y += V_StringHeight(waitinfo.node_names[i]);
	}
    }


  /*
    CL_DrawWaitInfo(&waitinfo);
  else
    {
      char *wait_message =
	"connected to server.\n\n"
	"waiting for controller to\n"
	"start the game.\n";
      
      V_WriteText(wait_message,
		  (SCREENWIDTH - V_StringWidth(wait_message)) / 2,
		  (SCREENHEIGHT - V_StringHeight(wait_message)) / 2);
    }
  */

}

//-------------------------------------------------------------------------
//
// Responder for key input
//

extern const char *shiftxform;    // hu_stuff.c

boolean CL_WaitResponder(event_t *ev)
{
  static boolean ctrldown = false;
  static boolean shiftdown = false;
  unsigned char ch;
  
  if(ev->data1 == KEYD_RCTRL)
    {
      ctrldown = ev->type == ev_keydown;
      return true;
    }

  if(ev->data1 == KEYD_RSHIFT)
    {
      shiftdown = ev->type == ev_keydown;
      return true;
    }
  
  if(ev->type != ev_keydown)
    return false;

  // special ctrl-enabled key combinations
  
  if(ctrldown)
    {
      // disconnect from server
      if(ev->data1 == 'd')
	{
	  char buffer[128];

	  sprintf(buffer, "disconnect from server?\n\n%s", s_PRESSYN);
	  MN_Question(buffer, "disconnect leaving");

	  // dont get stuck thinking ctrl is down
	  shiftdown = ctrldown = false;
	  return true;
	}
      
      // start game
      
      if(ev->data1 == KEYD_ENTER && got_waitinfo && waitinfo.controller)
	{
	  CL_SendStartGame();
	  // dont get stuck thinking ctrl is down
	  shiftdown = ctrldown = false;
	  return true;
	}

      // change muzak
      
      if(ev->data1 == 'm')
	{
	  S_StartRandomMusic();
	  return true;
	}
    }

  // send chat

  if(ev->data1 == KEYD_ENTER && chat_input[0])
    {
      // send to server
      CL_SendChatMsg(chat_input);
      chat_input[0] = '\0';              // clear chat input
      return true;
    }

  // chat backspace
  
  if(ev->data1 == KEYD_BACKSPACE && chat_input[0])
    {
      chat_input[strlen(chat_input) - 1] = '\0';
      return true;
    }

  ch = shiftdown ? shiftxform[ev->data1] : ev->data1;
  
  // chat input

  if(isprint(ev->data1) && strlen(chat_input) < CHAT_MAXINPUT)
    {
      chat_input[strlen(chat_input) + 1] = '\0';
      chat_input[strlen(chat_input)] = ch;
      return true;
    }

  return false;
}

//--------------------------------------------------------------------------
//
// CL_StartGame
//
// Called when we receive a pt_startgame packet
//

static void CL_StartGame(startgame_t *sg)
{
  int i;

  // check correct packet
  
  if(!CL_CorrectPacket(sg->packet_num))
    return;
  
  //  if(gamestate != GS_SERVERWAIT)
  //    return;
  
  C_Printf("console is %i of %i\n", sg->player, sg->num_players);

  gamestate = GS_CONSOLE;
  
  rngseed =
    sg->rndseed[0] +
    (sg->rndseed[1] << 8) +
    (sg->rndseed[2] << 16) +
    (sg->rndseed[3] << 24);

  for(i=0; i<sg->num_players; i++)
    {
      players[i].playerstate = PST_LIVE;
      playeringame[i] = true;
      nettics[i] = 0;
    }
  for(; i<MAXPLAYERS; i++)
    playeringame[i] = false;

  if(sg->player == -1)
    {
      // drone
      drone = true;
      consoleplayer = 0;
    }
  else
    consoleplayer = sg->player;

  // set ticdup

  ticdup = sg->ticdup;
  
  // find which node is the controller
  
  CL_FindController();
  
  // clear consistancy

  memset(consistancy, 0, sizeof(consistancy));
  
  // set tics
  
  levelstarttic = basetic = 0;
  gametic = 0;
  maketic = 1;

  ResetNet();
  
  C_SendNetData();
}

//-------------------------------------------------------------------------
//
// CL_WaitInfo
//
// Store new wait info sent from server
//

static void CL_WaitInfo(waitinfo_t *wi)
{
  if(!CL_CorrectPacket(wi->packet_num))
    return;  
  got_waitinfo = true;
  waitinfo = *wi;
}

static void CL_ChatMsg(msgpacket_t *msg)
{
  if(!CL_CorrectPacket(msg->packet_num))
    return;

  CL_ChatPrint(msg->message);
}

//=========================================================================
//
// Game Packets
//
// Deal with incoming game packets from the server carrying tics from
// the other nodes.
//
//=========================================================================

//-------------------------------------------------------------------------
//
// ExpandTics
//
// From the original networking code.
// To save space, we only send the low byte of the tic number.
// ExpandTics figures out the tic number from just the low byte.
//

static int ExpandTics (int low)
{
  int delta;

  delta = low - (maketic & 0xff);

  if (delta >= -64 && delta <= 64)
    return (maketic & ~0xff) + low;
  if (delta > 64)
    return (maketic & ~0xff) - 256 + low;
  if (delta < -64)
    return (maketic & ~0xff) + 256 + low;
      
  I_Error ("ExpandTics: strange value %i at maketic %i",low,maketic);
  return 0;
}

//-------------------------------------------------------------------------
//
// CL_CheckResend
//
// Send a tic resend request to the server if we have missed a tic
//

static void CL_CheckResendTics()
{
  if(!tic_resend)
    return;
  if(!netgame || demoplayback)
    return;

  // time out if we have sent a resend packet and got no response
  
  if(I_GetTime_RealTime() > tic_resendtime + latency)
    {  
      netpacket_t packet;
      svticresend_t *rp = &packet.data.svticresend;
      int i;

      //      C_Printf("client: send resend (waiting %i)\n", nettics[i]);
      
      packet.type = pt_svticresend;
      
      for(i=0; i<MAXPLAYERS; i++)
	{
	  if(!playeringame[i])
	    continue;
	  rp->ticnum[i] = (nettics[i] + 0) & 255;
	}

      SendPacket(&server, &packet);

      tic_resendtime = I_GetTime_RealTime();
    }
}

//-------------------------------------------------------------------------
//
// CL_GamePacket
//
// Deal with incoming tics forwarded by the server
//

static void CL_GamePacket(gamepacket_t *gp)
{
  int i;

  //  if(drone)
  //    C_Printf("client: got game packet\n");
  
  // copy out all tics
  
  if(gp->num_tics == 0)
    I_Error("packet contains no tics");
  
  for(i=0; i<gp->num_tics; i++)
    {
      int player = gp->tics[i].player;
      int ticnum = ExpandTics(gp->tics[i].ticnum);

      if(player == consoleplayer && !drone)
	I_Error("got local cmds from server");
      
      // check if tic expected
      
      if(ticnum != nettics[player] + 1)
	{
	  if(ticnum <= nettics[player])
	    {
	      // already got tic -- dont care
	      //	      C_Printf("client: dup tic (%i %i)\n", ticnum, nettics[player]);
	    }
	  else
	    {
	      // missed tic
	      //   C_Printf("client: missed tic (%i %i)\n", ticnum,
	      //            nettics[player]);
	      /********
		RESEND
	        ********/
	      tic_resend = true;
	    }
	  
	  continue;
	}

      //	C_Printf("client: correct tic %i\n", ticnum);

      tic_resend = false;     // no longer need resend
      tic_resendtime = -1;
      
      backup_tics[player][ticnum & 255] = gp->tics[i].ticcmd;
      nettics[player] = ticnum;
    }
}

//-------------------------------------------------------------------------
//
// CL_ResendTics
//
// Resend tics missed by server
//

static void CL_ResendTics(clticresend_t *tr)
{
  int ticnum = ExpandTics(tr->ticnum);
  int i;
  gamepacket_t gp;

  //  C_Printf("client: resend tics from %i\n", ticnum);
  
  gp.num_tics = 0;
  
  for(i=ticnum; i<maketic; i++)
    {
      // add next tic
      //      C_Printf("resend %i\n", i);
      gp.tics[gp.num_tics].player = consoleplayer;
      gp.tics[gp.num_tics].ticnum = i & 255;
      gp.tics[gp.num_tics].ticcmd = backup_tics[consoleplayer][i & 255];

      gp.num_tics++;

      if(gp.num_tics >= NUM_TICS)
	{
	  SendGamePacket(&server, &gp);
	  gp.num_tics = 0;
	}
    }

  // send any remaining tics
  
  if(gp.num_tics)
    SendGamePacket(&server, &gp);
}  

//=========================================================================
//
// Other types of packet
//
// The Server will send us other types of packet: console commands,
// signals that other players have quit etc.
//
//=========================================================================

//-------------------------------------------------------------------------
//
// CL_Speedup
//
// Speed up or slow down the game by an amount specified by the server.
// Uses skiptics - replacement for the old system which tried to adjust
// based on maketic and nettics of the key player
//

static void CL_Speedup(speeduppacket_t *speedup)
{
  if(!CL_CorrectPacket(speedup->packet_num))
    return;

  gametime += speedup->skiptics;

  C_Printf("skiptics: %i\n", speedup->skiptics);
}

//-------------------------------------------------------------------------
//
// CL_TextMsg
//
// Server can send text messages to clients which will be displayed on
// their screens/in consoles
//

static void CL_TextMsg(msgpacket_t *msg)
{
  if(!CL_CorrectPacket(msg->packet_num))
    return;
  
  doom_printf(FC_GRAY "%s", msg->message);
}

//-------------------------------------------------------------------------
//
// CL_AcceptPacket
//

static void CL_AcceptPacket(acceptpacket_t *ap)
{
  CL_CorrectPacket(ap->packet_num);
}

//-------------------------------------------------------------------------
//
// CL_PlayerQuit
//
// Called when player quits the game
//

static void CL_PlayerQuit(quitpacket_t *qp)
{
  int pl;
  
  pl = qp->player;
  
  doom_printf(FC_GRAY "\a%s quit (%s)", players[pl].name, qp->quitmsg);

  // spawn 

  if(gamestate == GS_LEVEL)
    P_RemoveMobj(players[pl].mo);
  
  playeringame[pl] = false;

  // re calculate which is controller
  
  CL_FindController();
}

//=========================================================================
//
// Ping
//
// We need a ping timing system so we can efficiently calculate
// the latency from the server, so we know when to resend a
// packet we have missed.
//
//=========================================================================

static int ping_sendtime;

//-------------------------------------------------------------------------
//
// CL_Ping
//
// Send pong (ping reply) to the server, which is pinging us.
//

static void CL_Ping()
{
  netpacket_t packet;

  packet.type = pt_pong;        // pong = ping response

  SendPacket(&server, &packet);
}

//-------------------------------------------------------------------------
//
// CL_Pong
//
// Response to our ping request
//

static void CL_Pong()
{
  latency = I_GetTime_RealTime() - ping_sendtime;
}

//-------------------------------------------------------------------------
//
// CL_CheckPing
//
// Check if its time to send another ping request to the server
//

static void CL_CheckPing()
{
  if(!netgame || demoplayback)
    return;

  if(I_GetTime_RealTime() > ping_sendtime + PING_FREQ * 35)
    {
      netpacket_t packet;

      ping_sendtime = I_GetTime_RealTime();
      
      packet.type = pt_ping;

      SendPacket(&server, &packet);
    }
}


//=========================================================================
//
// Packet Listening
//
// We listen for new packets from the server and call the appropriate
// functions to deal with them.
//
//=========================================================================

//-------------------------------------------------------------------------
//
// CL_NetPacket
//
// Deal with incoming netpacket_t's from server
//

static void CL_NetPacket(netpacket_t *packet)
{
  // got a packet

  lastpacket_time = I_GetTime_RealTime();

  //  C_Printf("client: got packet\n");
  switch(packet->type)
    {
    case pt_ack:                    // acknowledge received packet
      CL_AckPacket(&packet->data.ackpacket);
      break;
      
    case pt_accept:                 // accept connection to game
      CL_AcceptPacket(&packet->data.acceptpacket);
      break;
      
    case pt_gametics:               // game data
      CL_GamePacket(&packet->data.gamepacket);
      break;
      
    case pt_compressed:             // compressed game data
      CL_GamePacket
	(DecompressPacket(&packet->data.compressed));
      break;
      
    case pt_console:                // a console command
      // deal with console command: run/whatever
      break;
      
    case pt_quit:                   // remote node disconnecting
      CL_PlayerQuit(&packet->data.quitpacket);
      break;
      
    case pt_textmsg:                  // text message
      CL_TextMsg(&packet->data.messagepacket);
      break;

    case pt_shutdown:               // server shutdown
      CL_Disconnect("server shutdown");
      C_Printf(FC_GRAY "server shutdown\n");
      break;
      
    case pt_clticresend:              // resend tics
      CL_ResendTics(&packet->data.clticresend);
      break;

    case pt_speedup:                // speedup packet
      CL_Speedup(&packet->data.speedup);
      break;
      
    case pt_ping:                     // send ping reply
      CL_Ping();
      break;
      
    case pt_pong:                     // response to our ping request
      CL_Pong();
      break;
      
    case pt_waitinfo:                 // while-u-wait info
      CL_WaitInfo(&packet->data.waitinfo);
      break;
      
    case pt_startgame:                // signal to start game
      CL_StartGame(&packet->data.startgame);
      break;

    case pt_chat:                     // while-u-wait chat message
      CL_ChatMsg(&packet->data.messagepacket);
      break;
	
      // don't care about anything else
      
    default:
      break;
    }
}

//-------------------------------------------------------------------------
//
// CL_GetPackets
//
// Listen for new packets from the server.
// Pass any new packets to CL_NetPacket.
//

static void CL_GetPackets()
{
  // only get packets when in netgame
  
  while(netgame && !demoplayback)
    {
      netpacket_t *packet;
      int remote_node;
      
      packet = server.netmodule->GetPacket(&remote_node);
	
      // no more packets?
      if(!packet)
	break;
      
      // only care about packets from server
      // anything else can go to hell
      if(remote_node != server.node)
	continue;
      
      CL_NetPacket(packet);
    }
}

//=========================================================================
//
// Tic Building
//
// Every tic we need to build a new ticcmd and send it to the server.
//
//=========================================================================

//-------------------------------------------------------------------------
//
// GetConsistancy
//
// Get consistancy byte based on current player positions
//

byte GetConsistancy()
{
  byte consistancy = 0;
  int i;

  if(gamestate == GS_LEVEL)
    for(i=0; i<MAXPLAYERS; i++)
      {
	if(!playeringame[i])
	  continue;
	consistancy += players[i].mo->x;
	consistancy += players[i].mo->y;
      }

  return consistancy;
}

//-------------------------------------------------------------------------
//
// CL_BuildTiccmd
//
// Build a new ticcmd. If in a netgame, send it to the server.
//

static void CL_BuildTiccmd()
{
  ticcmd_t ticcmd;
  
  // build the new ticcmd
  
  V_StartTic();
  D_ProcessEvents();
  //	      if (maketic - gameticdiv >= BACKUPTICS/2-1)
  //		break;          // can't hold any more
  G_BuildTiccmd(&ticcmd);

  ticcmd.consistancy = consistancy[maketic % CONSISTANCY_BUFFER];
  
  backup_tics[consoleplayer][maketic & 255] = ticcmd;
  
  // if this is a netgame, send the new ticcmd to the server

  if(netgame && !demoplayback)
    {
      gamepacket_t gp;
      int starttic;
      int i;
      
      starttic = maketic - extratics;
      
      gp.num_tics = 0;

      // send the latest tic _and_ some of the previous tics
      // if a packet is lost, we still have the ticcmds duplicated
      // in the next packet
      
      for(i=starttic; i<=maketic; i++)
	{
	  gp.tics[gp.num_tics].player = consoleplayer;
	  gp.tics[gp.num_tics].ticnum = i & 255;
	  gp.tics[gp.num_tics].ticcmd =
	    backup_tics[consoleplayer][i & 255];

	  gp.num_tics++;
	}

      SendGamePacket(&server, &gp);
    }
  
  maketic++;
}

//------------------------------------------------------------------------
//
// CL_ReadTiccmds
//

void CL_ReadTiccmds()
{
  int i;

  for(i=0; i<MAXPLAYERS; i++)
    {
      if(!playeringame[i])
	continue;
      CL_ReadDemoCmd(&backup_tics[i][maketic & 255]);
      nettics[i]++;
    }
}

//-------------------------------------------------------------------------
//
// NetUpdate
//
// This is called almost continously, including in the rendering code,
// to ensure accurate sync. We check the clock to see if it is time
// to make any new tics, and if so, send them. We also check for any
// new packets from the server and if so, deal with them.
//
// sf: tic building is complicated because of ticdup. Each time we
// enter NetUpdate, we find the time since the last time it was called.
// We store the elapsed time in newtics. We then adjust newtics because
// of skiptics. newtics is then added to buildabletics. When
// buildabletics >= ticdup, we can make buildabletics/ticdup new ticcmds.
//

void NetUpdate()
{
  int entertic = I_GetTime();
  int newtics = (entertic - gametime) / ticdup;

  // singletic update is syncronous
  
  if(singletics) 
    return;
  
  if(newtics > 0)
    {
      if(newtics > 4)
	newtics = 4;

      gametime += newtics * ticdup;
      
      if(!drone && gamestate != GS_SERVERWAIT &&
	 (maketic - gametic/ticdup) < 20)
	{
	  int ticnum;
	  	  
	  for (ticnum=0 ; ticnum<newtics ; ticnum++)
	    if(demoplayback)
	      CL_ReadTiccmds();
	    else
	      CL_BuildTiccmd();
	}
    }

  // drones do not build ticcmds
  
  if(netgame && !demoplayback)
    {  
      // get all new packets from server
      
      CL_GetPackets();
      
      // check for packet timeout
      
      CL_CheckResend();
      
      // check if we need server to resend tics
      
      CL_CheckResendTics();
      
      // find latency sometimes
      
      CL_CheckPing();
  
      // update server
      
      SV_Update();
    }
}

//-------------------------------------------------------------------------
//
// ResetNet
//
// If we (for example) load a new level, it can sometimes take
// quite a long time to load. When it is finished, NetUpdate
// will suddenly make and send many tics to make up for the
// lost time. This can overload the other computer. ResetNet
// resets the tic timer to prevent this happening.
//

void ResetNet()
{
  gametime = I_GetTime();
}

//=========================================================================
//
// Other types of Packet Send
//
//=========================================================================


//==========================================================================
//
// Startup Init Network
//
//==========================================================================

//----------------------------------------------------------------------------
//
// D_InitPlayers
//
// sf: init players, set names, skins, colours etc

static void D_InitPlayers (void)
{
  int i;
  
  for(i=0; i<MAXPLAYERS; i++)
    {
      sprintf(players[i].name, "player %i", i+1);
      players[i].colormap = i % TRANSLATIONCOLOURS;
      players[i].skin = &marine;
    }
}

//---------------------------------------------------------------------------
//
// D_QuitNetGame
//
// Called on shutdown. Shutdown server properly and send proper
// disconnect message to server.
//

static void D_QuitNetGame()
{
  NetDisconnect("exitting doom");
}

//--------------------------------------------------------------------------
//
// D_InitNetGame
//
// Called on startup.
//

void D_InitNetGame()
{
  gametic = maketic = 0;
  playeringame[0] = true;      // local connection
  ticdup = 1;

  D_InitPlayers();
  SV_ClearModules();

  SV_Init();
  
#ifdef TCPIP
  // init udp library
  UDP_InitLibrary();

  // try to resolve something - djgpp libsocket displays stupid
  // text mode messages the first time
  UDP_Resolve("localhost");    
#endif
  
  atexit(D_QuitNetGame);
}

//=========================================================================
//
// Tic Running
//
//=========================================================================

//-------------------------------------------------------------------------
//
// TryRunTics
//
// Run new gametics. Returns true if any tics were run.
//

static boolean RunGameTics()
{
  int i, n;
  int lowtic = MAXINT;
  int availabletics;
  int count;
  int key = -1;
  int consist;

  // if timing demo, read the ticcmds in here rather than in
  // NetUpdate
  
  if(singletics)
    {
      // read ticcmds
      CL_ReadTiccmds();
      availabletics = 1;
    }
  else
    {
      // normal tic stuff

      NetUpdate();
  
      // find lowest tic we can run
      
      for(i=0; i<MAXPLAYERS; i++)
	{
	  if(!playeringame[i])
	    continue;
	  if(key == -1)
	    key = i;
	  if(nettics[i] < lowtic)
	    lowtic = nettics[i];
	}
      
      // set open socket if we havent received packets from server
      // for a while
      
      opensocket =
	netgame && !demoplayback &&
	(maketic - gametic >= 20) &&
	(I_GetTime_RealTime() - lastpacket_time > 8);
      
      // find maximum number of tics we can run
      
      availabletics = lowtic - gametic/ticdup; // - 1;
      
      if(availabletics < 0)
	{
	  //  C_Printf("availabletic < 0! %i %i\n", lowtic, gametic/ticdup);
	  return false;
	}
      
      if(availabletics == 0)
	{
	  return false;        // wait until we have some tics
	}
    }
  
  // try and run tics now

  count = availabletics;

  while(count--)
    {
      // copy tics into player_cmds
      // check for consistancy errors

      consist = -1;
      out_of_sync = false;
      
      for(i=0; i<MAXPLAYERS; i++)
	{
	  if(!playeringame[i])
	    continue;
	  player_cmds[i] = backup_tics[i][(gametic/ticdup) & 255];
	  if(consist == -1)
	    consist = player_cmds[i].consistancy;
	  else
	    // flash out-of-sync warning when we go out of sync in netgames
	    if(consist != player_cmds[i].consistancy)
	      out_of_sync = netgame && !demoplayback;
	}
      
      for(i=0; i<ticdup; i++)
	{
	  // save consistancy 
	  consistancy[(gametic/ticdup) % CONSISTANCY_BUFFER]
	    = GetConsistancy();
	  
	  G_Ticker ();
	  if (advancedemo)
	    D_DoAdvanceDemo ();
	  //      C_Printf("run tic %i!\n", gametic);
	  
	  gametic++;

	  // clear some of the data from the ticcmds

	  if(i == 0)
	    for(n=0; n<MAXPLAYERS; n++)
	      {
		if (player_cmds[n].buttons & BT_SPECIAL)
		  player_cmds[n].buttons = 0;
		memset(player_cmds[n].consdata, 0,
		       sizeof(player_cmds[n].consdata));
	      }
	}
    }

  // Movement prediction
  // reduce laggy feel

  if(netgame && !demoplayback && gamestate == GS_LEVEL)
    {
      // sf: only run multiples of 2 tics
      // otherwise we can end up flicking between eg. 4 and 5 and it jumps
      availabletics = (maketic*ticdup - gametic);
      
      if(prediction_threshold < 16 &&            // 16 = predict max
	 availabletics > prediction_threshold)
	availabletics = prediction_threshold;
      
      // doom_printf("tics predicted: %i", availabletics);
      
      if(availabletics)
	{
	  int ticnum;
	  
	  P_StartPrediction(&players[consoleplayer]);

	  ticnum = gametic;
	  
	  for(i=0; i<availabletics; i++)
	    {
	      P_RunPredictedTic
		(&backup_tics[consoleplayer][(ticnum/ticdup) & 255]);
	      ticnum++;
	    }
	}
      
      //  I_Error("TryRunTics");
    }
  
  return true;       // ran some tics
}


//---------------------------------------------------------------------------
//
// RunEnvTics
//
// run tics for 'environment' -- menu, console etc.
// returns true if it ran some tics
// We run this seperately from the Game tickers -- if the game freezes up,
// we can still use the menu and type at the console fine.
//

static int env_exittic = 0;

static boolean RunEnvTics ()
{
  // gettime_realtime is used because the game speed can be changed
  int entertic = I_GetTime_RealTime();
  int realtics = entertic - env_exittic;
  int i;

  if(realtics <= 0)
    {
      return false;      // no tics to run 
    }    

  // sf: run the menu and console regardless of 
  // game time. to prevent lockups

  for(i = 0; i<realtics; i++)   // run tics
    {
      // all independent tickers here
      MN_Ticker ();
      C_Ticker ();
      V_FPSTicker();
      if(gamestate == GS_LEVEL)
	{
	  if((walkcam_active = camera==&walkcamera))
	    P_WalkTicker();
	}
      if(inwipe)
	Wipe_Ticker();
    }

  env_exittic = entertic;  // save for next time

  return true; // ran some tics
}

//--------------------------------------------------------------------------
//
// TryRunTics
//
// Called in main game loop.
// Run new tics: will not return until at least one game or environment
// tic has been run.
//

void TryRunTics (void)
{
  // we call the respective functions above
  // run the game tickers and environment tickers

  // if one of them managed to run a tic,
  // something may have changed (visually)
  // we do not exit the function until something
  // new has happened
  
  while(true)
    {
      // run these here now to get keyboard
      // input for console/menu
      
      V_StartTic ();
      D_ProcessEvents ();
     
      // run tickers
      // do not exit loop until we have run at least one tic

      if(RunEnvTics() + (gamestate != GS_SERVERWAIT ? RunGameTics() : 0)  > 0)
	break;

      // free up time to the os
      I_Sleep(10000);
    }
}

//===========================================================================
//
// Console Commands
//
//===========================================================================

CONSOLE_COMMAND(connect, cf_buffered)
{
  netnode_t *node;

  if(!c_argc)
    {
      C_Printf("usage: connect <hostname>\n");
    }
  else
    {
      node = Net_Resolve(c_argv[0]);

      if(node)
	CL_Connect(node);
      else
	C_Printf("unknown location\n");
    }
}

CONSOLE_COMMAND(disconnect, cf_netonly)
{
  char *reason = c_args;

  while(*reason == ' ')
    reason++;

  if(!*reason)
    reason = "disconnecting";
  
  NetDisconnect(reason);
}

CONSOLE_COMMAND(latency, 0)
{
  C_Printf("latency to server: %i tics\n", latency);
}

CONSOLE_COMMAND(restart, 0)
{
  CL_SendStartGame();
}

CONSOLE_INT(prediction, prediction_threshold, NULL, 0, 16, NULL, 0) {}

CONSOLE_INT(ticdup, pending_ticdup, NULL, 0, 6, NULL, 0) {}

void CL_Demo_AddCommands();                  // cl_demo.c

void CL_AddCommands()
{
  CL_Demo_AddCommands();
  
  C_AddCommand(connect);
  C_AddCommand(disconnect);
  C_AddCommand(latency);
  C_AddCommand(restart);

  C_AddCommand(ticdup);
  
  C_AddCommand(prediction);
}

//--------------------------------------------------------------------------
//
// $Log$
// Revision 1.11  2000-05-12 16:41:59  fraggle
// even better speeddup algorithm
//
// Revision 1.10  2000/05/10 13:11:37  fraggle
// fix demos
//
// Revision 1.9  2000/05/07 13:40:31  fraggle
// default to full prediction
//
// Revision 1.8  2000/05/07 13:11:21  fraggle
// improve multiplayer chatroom interface
//
// Revision 1.7  2000/05/06 14:39:10  fraggle
// add prediction/ticdup to menu
//
// Revision 1.6  2000/05/06 14:06:11  fraggle
// fix ticdup
//
// Revision 1.5  2000/05/03 16:46:45  fraggle
// check wads in netgames
//
// Revision 1.4  2000/05/03 16:30:42  fraggle
// remove multiplayer quit flash
//
// Revision 1.3  2000/05/03 16:21:23  fraggle
// client speedup code
//
// Revision 1.2  2000/05/02 15:43:40  fraggle
// client movement prediction
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//--------------------------------------------------------------------------
