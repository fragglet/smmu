// Emacs style mode select -*- C++ -*-
//--------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
// Some parts from the sersetup source - (C)id
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
// SMMU Networking Serial Module
//
// Network module for dos serial/modem connections
//
// By Simon Howard/portions from sersetup
//
//---------------------------------------------------------------------------

#include "ser_port.h"

#include "../c_io.h"
#include "../c_runcmd.h"
#include "../doomdef.h"
#include "../d_main.h"
#include "../v_misc.h"
#include "../sv_serv.h"
#include "../z_zone.h"

#define COMMAND_COLOUR FC_GOLD
#define RESPONSE_COLOUR FC_GREEN

//==========================================================================
//
// Serial Functions
//
//==========================================================================

extern int comport;

//--------------------------------------------------------------------------
//
// Ser_ReadPacket
//
// Read a new packet from the port
//

#define MAXPACKET	512
#define	FRAMECHAR	0x70

static char packet[MAXPACKET];
static int packetlen;
static boolean inescape;
static boolean newpacket;

static boolean Ser_ReadPacket (void)
{
  int c;
	
  // if the buffer has overflowed, throw everything out
	
  if (inque.head-inque.tail > QUESIZE - 4)	// check for buffer overflow
    {
      inque.tail = inque.head;
      newpacket = true;
      return false;
    }
  
  if (newpacket)
    {
      packetlen = 0;
      newpacket = false;
    }
  
  do
    {
      c = read_byte ();
      if (c < 0)
	return false;		// haven't read a complete packet
      //printf ("%c",c);
      if (inescape)
	{
	  inescape = false;
	  if (c != FRAMECHAR)
	    {
	      newpacket = true;
	      return true;	// got a good packet
	    }
	}
      else if (c==FRAMECHAR)
	{
	  inescape = true;
	  continue;	// don't know yet if it is a terminator
	}	        // or a literal FRAMECHAR
      
      if (packetlen >= MAXPACKET)
	continue;			// oversize packet
      packet[packetlen] = c;
      packetlen++;
    } while (1);
}

//--------------------------------------------------------------------------
//
// Ser_WriteBuffer
//
// Write a buffer to the serial port
//

static void Ser_WriteBuffer( char *buffer, unsigned int count )
{
  // if this would overrun the buffer, throw everything else out
  if (outque.head-outque.tail+count > QUESIZE)
    outque.tail = outque.head;
  
  while (count--)
    write_byte (*buffer++);
  
  if ( INPUT( uart + LINE_STATUS_REGISTER ) & 0x40)
    jump_start();
}

//-------------------------------------------------------------------------
//
// Ser_WritePacket
//
// Write a buffer to the serial port
//

// sf: made void *buffer not char *buffer

static void Ser_WritePacket (void *buffer, int len)
{
  int b;
  static char localbuffer[MAXPACKET*2+2];
  char *buf = buffer;

  b = 0;
  if (len > MAXPACKET)
    return;
	
  while (len--)
    {
      if (*buf == FRAMECHAR)
	localbuffer[b++] = FRAMECHAR;	// escape it for literal
      localbuffer[b++] = *buf++;
    }
  
  localbuffer[b++] = FRAMECHAR;
  localbuffer[b++] = 0;
  
  Ser_WriteBuffer (localbuffer, b);
}

//==========================================================================
//
// Modem
//
//==========================================================================

// phone number to dial

char *phone_number;

// other variables

static boolean usemodem;
static boolean waiting_call;          // waiting for call
static boolean connected;             // modem dialup active?

// modem setup commands (modem.cfg)

static char *initstring;
static char *shutdownstring;

//--------------------------------------------------------------------------
//
// ModemClear
//
// Clear out modem responses from previous connections
//

static void ModemClear ()
{
  long starttime = I_GetTime_RealTime();
  int i=0, c;
  
  while(1)
    {
      c = read_byte();
      if(c != -1)
	{
	  i++;
	  starttime = I_GetTime_RealTime(); // reset
	}
      else
	if(I_GetTime_RealTime() > starttime + 35)
	  break;
    }
  packetlen = newpacket = inescape = 0;      // clear it
}

//---------------------------------------------------------------------------
//
// ModemCommand
//
// Send a command to the modem
//

void ModemCommand(char *command)
{
  usermsg (COMMAND_COLOUR "%s", command);
  Ser_WriteBuffer(command, strlen(command));
  Ser_WriteBuffer("\r",1);
}

//---------------------------------------------------------------------------
//
// ModemResponse
//
// Check for a particular response from the modem.
// Returns response (read from modem) if correct response read
//

#define MAXRESPONSE 128

static char response_buffer[MAXRESPONSE] = "";

char *ModemResponse(char *resp)
{
  char c;
  static char read_response[MAXRESPONSE];   
  
  while( (c = read_byte()) != -1)
    {
      if(c == '\n')        // new line
	{
	  // check if line matches response
	  if(!strncasecmp(response_buffer, resp, strlen(resp)))
	    {
	      strcpy(read_response, response_buffer);
	      response_buffer[0] = '\0';
	      return response_buffer;
	    }
	  
	  // clear response buffer
	  response_buffer[0] = '\0';
	  continue;
	}
      
      if(!isprint(c))      // only printable chars
	continue;
      
      response_buffer[strlen(response_buffer) + 1] = '\0';
      response_buffer[strlen(response_buffer)] = c;
    }

  return NULL;       // response not got
}

//---------------------------------------------------------------------------
//
// Modem_WaitResponse
//
// Modem_Response but waits until the response is got
//

#define TIMEOUT 35*5

static boolean Modem_WaitResponse(char *resp)
{
  int timeout = I_GetTime_RealTime() + TIMEOUT;

  while(I_GetTime_RealTime() < timeout)
    {
      if(ModemResponse(resp))
	return true;
    }

  return false;
}

//---------------------------------------------------------------------------
//
// Modem_Hangup
//
// Hang up the phone and disconnect
//

static void Modem_Hangup()
{
  if(usemodem && connected)
    {
      usermsg("Dropping DTR.. ");

      OUTPUT( uart + MODEM_CONTROL_REGISTER,
	      INPUT( uart + MODEM_CONTROL_REGISTER ) & ~MCR_DTR );
      delay (1250);
      OUTPUT( uart + MODEM_CONTROL_REGISTER,
	      INPUT( uart + MODEM_CONTROL_REGISTER ) | MCR_DTR );
      
      // hang up modem
      
      ModemCommand("+++");
      delay (1250);
      ModemCommand(shutdownstring);
      delay (1250);

      connected = false;
    }
}

//---------------------------------------------------------------------------
//
// Ser_Dial
//
// Phone up another computer
//

void Ser_Dial()
{
  char cmd[MAXRESPONSE];
  char *connect_resp = NULL;
  
  usemodem = true;

  usermsg ("Dialing...");

  // send dial
  
  sprintf (cmd, "ATDT%s", phone_number);
  ModemCommand(cmd);
  
  while(!connect_resp)
    {
      // TODO: abort sequence!

      connect_resp = ModemResponse ("CONNECT");
    }

  if(strncmp(connect_resp+8,"9600",4) )
    {
      usermsg ("The Connection MUST be made at 9600\n"
	       "baud, no Error correction, no compression!\n"
	       "Check your modem initialization string!");
      Modem_Hangup();
      return;
    }

  connected = true;
}

//---------------------------------------------------------------------------
//
// Ser_WaitForCall
//
// Wait for someone to phone up
//

void Ser_WaitForCall()
{
  usemodem = true;
  waiting_call = true;
  connected = false;
}

//---------------------------------------------------------------------------
//
// Ser_AnswerCall
//
// Answer the phone
//

static void Ser_AnswerCall()
{
  usermsg("the phone is ringing");
  
  ModemCommand ("ATA");
  ModemResponse ("CONNECT");

  connected = true;
}

//-------------------------------------------------------------------------
//
// Modem .cfg file
//
// Called on startup - read init and shutdown strings from modem.cfg
//

static char *ReadLine(FILE *f)
{
  static char read_buffer[128];
  char c;

  read_buffer[0] = '\0';       // empty string
  
  do
    {
      c = fgetc (f);
      if (c == EOF)
	{
	  usermsg ("EOF in modem.cfg");
	  return "";
	}
      if (c == '\r' || c == '\n')
	break;

      // add char
      if(isprint(c))
	{
	  read_buffer[strlen(read_buffer) + 1] = '\0';
	  read_buffer[strlen(read_buffer)] = c;
	}
      
    } while (1);
  
  return read_buffer;
}


void Ser_ReadModemCfg()
{
  FILE *f;
  
  f = fopen ("modem.cfg", "r");
  if (!f)
    {
      usermsg ("Couldn't read MODEM.CFG");
      initstring = "atz";
      shutdownstring = "at z h0";
      return;
    }

  usermsg("modem.cfg read");
  initstring = strdup(ReadLine(f));
  shutdownstring = strdup(ReadLine(f));
  fclose (f);
}

//===========================================================================
//
// Module Functions
//
// The Serial netmodule
//
//===========================================================================

// netmodules.
// we have a different netmodule for serial and modem links

boolean Ser_Init();
boolean Modem_Init();
void Ser_Disconnect();
void *Ser_GetPacket(int *node);
void Ser_SendPacket(int node, void *data, int datalen);
void Ser_SendBroadcast(void *data, int datalen);

netmodule_t serial =
  {
    "serial",
    Ser_Init,
    Ser_Disconnect,
    Ser_SendPacket,
    Ser_SendBroadcast,
    Ser_GetPacket,
  };

netmodule_t modem =
  {
    "modem",
    Modem_Init,
    Ser_Disconnect,
    Ser_SendPacket,
    Ser_SendBroadcast,
    Ser_GetPacket,
  };

//-------------------------------------------------------------------------
//
// Ser_Init
//

boolean Ser_Init()
{
  InitPort ();
  ModemClear();

  usemodem = waiting_call = false;
  
  serial.initted = modem.initted = true;
  serial.numnodes = modem.numnodes = 2;

  // initted ok

  return true;
}

//-------------------------------------------------------------------------
//
// Ser_Disconnect
//

void Ser_Disconnect()
{
  // wait a bit for the ISR to finish sending the disconnect packet

  delay(300);
  
  // hang up and shut down

  Modem_Hangup();
  ShutdownPort();

  connected = false;
  serial.initted = modem.initted = false;
}

//---------------------------------------------------------------------------
//
// Modem_Init
//

boolean Modem_Init()
{
  if(!Ser_Init())
    return false;
  
  C_Printf("init: '%s'\n", initstring);
  ModemCommand(initstring);

  if(!Modem_WaitResponse("OK"))
    {
      Ser_Disconnect();
      return false;
    }
  
  usemodem = true;

  return true;
}

//---------------------------------------------------------------------------
//
// Ser_SendPacket
//

void Ser_SendPacket(int node, void *data, int datalen)
{
  Ser_WritePacket (data, datalen);
}

//--------------------------------------------------------------------------
//
// Ser_SendBroadcast
//

void Ser_SendBroadcast(void *data, int datalen)
{
  Ser_SendPacket(0, data, datalen);
}

//--------------------------------------------------------------------------
//
// Ser_GetPacket
//

void *Ser_GetPacket(int *node)
{
  if(usemodem && waiting_call)
    {
      // check for connect signal from modem
      if(ModemResponse("RING"))
	{
	  Ser_AnswerCall();
	  waiting_call = false;
	}

      return NULL;
    }

  if(Ser_ReadPacket ())
    {
      if(node)
	*node = 1;
      return packet;
    }

  return NULL;     // no packet
}

//=========================================================================
//
// Console Commands
//
//=========================================================================

CONSOLE_INT(ser_comport, comport, NULL,             1, 4, NULL, 0) {}
CONSOLE_STRING(ser_phonenum, phone_number, NULL,    25, 0) {}

void Ser_AddCommands()
{
  C_AddCommand(ser_comport);
  C_AddCommand(ser_phonenum);
}

//---------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:12  fraggle
// Initial revision
//
//
//---------------------------------------------------------------------------
