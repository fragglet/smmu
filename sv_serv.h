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
// Networking Server Portion
//
// By Simon Howard
//
//---------------------------------------------------------------------------

#ifndef SV_SERV_H
#define SV_SERV_H

#include "doomdef.h"
#include "d_ticcmd.h"

typedef struct netmodule_s netmodule_t;
typedef struct netnode_s netnode_t;
typedef struct netpacket_s netpacket_t;
typedef struct tic_s tic_t;
typedef struct gamepacket_s gamepacket_t;
typedef struct fingerpacket_s fingerpacket_t;
typedef struct resendpacket_s resendpacket_t;

// how often we send a ping to update latency

#define PING_FREQ 30

#define MAXNETNODES 16
#define BACKUPTICS 12

//-------------------------------------------------------------------------
//
// Net Modules
//
// Each method of network communication has a different netmodule_t
// eg, one for serial, external, udp etc.

struct netmodule_s
{
  //------------------------------------
  // module name
  //------------------------------------

  char *name;
  
  //------------------------------------
  // init functions
  //------------------------------------

  // init comms
  // returns true if correctly initialised
  
  boolean (*Init)();

  // shutdown

  void (*Shutdown)();
  
  //------------------------------------
  // functions used for communication
  //------------------------------------

  // send packet

  void (*SendPacket)(int node, void *data, int datalen);

  // send broadcast packet

  void (*SendBroadcast)(void *data, int datalen);
  
  // get packet
  
  void *(*GetPacket)(int *node);

  // other info
  
  boolean initted;        // whether module is initialised
  int numnodes;           // number of nodes we can send to
};

// send address for broadcast 
#define NODE_BROADCAST (-1)

#define MAXNETMODULES 10

extern netmodule_t *netmodules[MAXNETMODULES];

#define MAXNODES 16

struct netnode_s
{
  netmodule_t *netmodule;           // network module used for comms
  int node;                         // node num
};


//--------------------------------------------------------------------------
//
// Tics.
//

struct tic_s
{
  byte player;
  byte ticnum;
  ticcmd_t ticcmd;
};

// length of the header of a gamepacket_t
#define GAMEHEADERLEN (int) &(((gamepacket_t *) 0)->tics)

#define NUM_TICS 8

struct gamepacket_s
{
  byte num_tics;
  tic_t tics[NUM_TICS];
};

typedef struct
{
  byte num_tics;
  byte data[1024];
} compressedpacket_t;

// resend tics


// client tic resend
typedef struct
{
  byte ticnum;
} clticresend_t;

// server tic resend
typedef struct
{
  byte ticnum[MAXPLAYERS];     // one for each player
} svticresend_t;

// speedup/slowdown packets

typedef struct
{
  byte packet_num;
  char skiptics;
} speeduppacket_t;

//--------------------------------------------------------------------------
//
// Finger server.
//
// Remote clients (ie. ones not in the game) can send a finger request to
// the server and have information sent back
//

// this is only for the reply, in the finger packet sent to the server
// packet->data is empty

struct fingerpacket_s
{
  byte players;             // number of players connected
  byte accepting;           // currently accepting connections?
  char server_name[50];     // name of server
  // other boring crap here
};

//-------------------------------------------------------------------------
//
// Reliable Send
//

// acknowledge packet
typedef struct
{
  byte packet;
} ackpacket_t;

//-------------------------------------------------------------------------
//
// Text message
//
// To allow server to send text message to clients, mainly
//

typedef struct
{
  byte packet_num;           // reliable send
  char message[50];
} msgpacket_t;

//-------------------------------------------------------------------------
//
// Console command packet
//

typedef struct
{
  byte cmdnum;             // console command to run
  byte dest;               // player to run it
  char args[50];           // arguments to command
} consolepacket_t;

//-------------------------------------------------------------------------
//
// Joining Server
//

// info sent to server when we join

typedef struct
{
  short version;        // game version -- must all be the same
  byte drone;           // set to non-zero if we want to be a drone
  byte wadsig[4];       // wad signature
  char name[20];        // node name
} joinpacket_t;

// send back the reason why comp can't join

typedef struct
{
  char reason[50];
} denypacket_t;

// data sent to clients while they wait for players to connect

typedef struct
{
  byte packet_num;                 // reliable send
  byte nodes;
  byte controller;                 // true if destination is game controller
  char node_names[MAXNETNODES][20];
} waitinfo_t;

// packet we send to client when we accept their connecting to server

typedef struct
{
  byte packet_num;                     // must ack packet
  char server_name[50];
} acceptpacket_t;

// we send a pt_startgame signal to start the game
// we include some other info in the packet

typedef struct
{
  byte packet_num;
  char player;             // player number allocated for this computer
  byte num_players;        // number of players in game
  byte rndseed[4];         // long
} startgame_t;

// sent when node disconnects from server/player exits game

typedef struct
{
  byte packet_num;              // must be in order
  char player;                  // = -1 if drone
  char quitmsg[50];
} quitpacket_t;

//-------------------------------------------------------------------------
//
// netpacket_t
//
// basic netpacket

// length of the header of a netpacket_t
#define HEADERLEN (int) &(((netpacket_t *) 0)->data)

// packet type:

enum
  {
    // game packets
    pt_gametics,               // game data
    pt_compressed,             // compressed game data

    pt_speedup,                // speedup packet
    
    pt_clticresend,            // client tic resend
    pt_svticresend,            // server tic resend
    
    // ping
    pt_ping,
    pt_pong,                   // ping response
    
    // other types of game packet
    pt_console,                // *UNUSED* a console command
    pt_ack,                    // ack packet - acknowledge packet received
    
    // other packets
    pt_quit,                   // player quit
    pt_shutdown,               // shutdown server
    pt_fingerrequest,          // finger request
    pt_finger,                 // finger reply w/data
    pt_textmsg,                // send a text message to the server
    
    // join packets
    pt_join,                   // connect to server
    pt_accept,                 // accept connection to server
    pt_deny,                   // deny connection
    pt_waitinfo,               // info about players connected
    pt_startgame,              // signal to start game
    pt_chat,                   // chat message - uses msgpacket_t
    // etc
  };

struct netpacket_s
{
  // packet type: see above
  
  byte type;   

  // data contained in the packet

  union
  {
    // for reliable packet send:
    // any packets sent reliably must have packet_num as their first byte
    byte packet_num;                

    gamepacket_t gamepacket;            // game tics
    compressedpacket_t compressed;      // compressed game tics

    speeduppacket_t speedup;            // speedup packet
    
    // resend tics:
    clticresend_t clticresend;          // client tic resend    
    svticresend_t svticresend;          // server tic resend
    
    ackpacket_t ackpacket;              // acknowledge packet received
        
    fingerpacket_t fingerpacket;        // finger data response
    msgpacket_t messagepacket;          // text message
    quitpacket_t quitpacket;            // node quit game
    
    // quit message
    // etc.
     
    startgame_t startgame;              // signal to start game
    acceptpacket_t acceptpacket;        // sent to successful joining client
    joinpacket_t joinpacket;            // data sent when we join
    denypacket_t denypacket;            // sent when server denies connection
    waitinfo_t waitinfo;                // info about server while waiting
  } data;
};

// number of packets to remember for missed packet resend
#define BACKUP_PACKETS 256

// server start/shutdown functions

void SV_Init();
void SV_Shutdown();

// functions to change modules server listens to:

void SV_ClearModules();
boolean SV_AddModule(netmodule_t *netmodule);

// called to check for new packets for server

void SV_Update();

#endif /* SV_SERV_H */

//---------------------------------------------------------------------------
//
// $Log$
// Revision 1.3  2000-05-03 16:46:45  fraggle
// check wads in netgames
//
// Revision 1.2  2000/05/03 16:21:23  fraggle
// client speedup code
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//---------------------------------------------------------------------------

