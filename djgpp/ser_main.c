// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// sersetup.c
//
// converted so that it is now inside the exe
// this is a hacked up piece of shit
//
// By Simon Howard, based on id sersetup source
//
//----------------------------------------------------------------------------

#include "../doomdef.h"
#include "../doomstat.h"

#include "ser_main.h"

#include "../c_io.h"
#include "../c_runcmd.h"
#include "../c_net.h"
#include "../d_event.h"
#include "../d_main.h"
#include "../d_net.h"
#include "../g_game.h"
#include "../i_video.h"
#include "../mn_engin.h"

extern void (*netdisconnect)();

int CheckForEsc();
void Ser_Disconnect();
void ModemClear ();

extern	que_t		inque, outque;

void jump_start( void );
extern int uart;

int usemodem;
char startup[256], shutdown[256];

connect_t connectmode=CONNECT;
char phonenum[50];

extern doomcom_t *doomcom;
doomcom_t ser_doomcom;

void ModemCommand (char *str);

int ser_active;

/*
================
=
= write_buffer
=
================
*/

void write_buffer( char *buffer, unsigned int count )
{
  // if this would overrun the buffer, throw everything else out
  if (outque.head-outque.tail+count > QUESIZE)
    outque.tail = outque.head;
  
  while (count--)
    write_byte (*buffer++);
  
  if ( INPUT( uart + LINE_STATUS_REGISTER ) & 0x40)
    jump_start();
}


/*
=================
=
= Ser_Error
=
= For abnormal program terminations
=
=================
*/

void Ser_Error (char *error, ...)
{
  va_list argptr;
  
  Ser_Disconnect();
  
  ShutdownPort ();
  
  if (error)
    {
      char tempstr[100];
      va_start (argptr,error);
      vsprintf (tempstr,error,argptr);
      usermsg("");
      usermsg(tempstr);
      usermsg("");
      va_end (argptr);
      
      // copy error message to menu
      strcpy(menu_error_message, tempstr);
      menu_error_time = -1; // infinite duration
    }
  ser_active = false;
}

void Ser_Disconnect()
{
  doomcom = &singleplayer;
	
  if(!usemodem) return;
	
  usermsg("");
  usermsg("Dropping DTR.. ");
  usermsg("");
	
  OUTPUT( uart + MODEM_CONTROL_REGISTER,
	  INPUT( uart + MODEM_CONTROL_REGISTER ) & ~MCR_DTR );
  delay (1250);
  OUTPUT( uart + MODEM_CONTROL_REGISTER,
	  INPUT( uart + MODEM_CONTROL_REGISTER ) | MCR_DTR );
  
  // hang up modem

  ModemCommand("+++");
  delay (1250);
  ModemCommand(shutdown);
  delay (1250);
}


/*
================
=
= Ser_ReadPacket
=
================
*/

#define MAXPACKET	512
#define	FRAMECHAR	0x70

char packet[MAXPACKET];
int packetlen;
int inescape;
int newpacket;

boolean Ser_ReadPacket (void)
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
      newpacket = 0;
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
	  if (c!=FRAMECHAR)
	    {
	      newpacket = 1;
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


/*
=============
=
= Ser_WritePacket
=
=============
*/

void Ser_WritePacket (char *buffer, int len)
{
  int b;
  char static localbuffer[MAXPACKET*2+2];
	
  b = 0;
  if (len > MAXPACKET)
    return;
	
  while (len--)
    {
      if (*buffer == FRAMECHAR)
	localbuffer[b++] = FRAMECHAR;	// escape it for literal
      localbuffer[b++] = *buffer++;
    }
  
  localbuffer[b++] = FRAMECHAR;
  localbuffer[b++] = 0;
  
  write_buffer (localbuffer, b);
}


/*
=============
=
= Ser_NetISR
=
=============
*/

void Ser_NetISR (void)
{
  if (ser_doomcom.command == CMD_SEND)
    {
      Ser_WritePacket ((char *)&ser_doomcom.data, ser_doomcom.datalength);
    }
  else if (ser_doomcom.command == CMD_GET)
    {      
      if (Ser_ReadPacket () && packetlen <= sizeof(ser_doomcom.data) )
	{
	  ser_doomcom.remotenode = 1;
	  ser_doomcom.datalength = packetlen;
	  memcpy (&ser_doomcom.data, &packet, packetlen);
	}
      else
	ser_doomcom.remotenode = -1;
    }
}




/*
=================
=
= Ser_Connect
=
= Figures out who is player 0 and 1
=================
*/

void Ser_Connect (void)
{
  int time;
  int oldsec;
  int localstage, remotestage;
  char str[20];
	
  //
  // wait for a good packet
  //
  
  usermsg("");
  usermsg("Attempting to connect across serial");
  usermsg("link, press escape to abort.");
  usermsg("");
	
  oldsec = -1;
  localstage = remotestage = 0;
  
  do
    {
      if(CheckForEsc())
	{
	  Ser_Error("connection aborted.");
	  return;
	}
      while (Ser_ReadPacket ())
	{
	  packet[packetlen] = 0;
	  //                      printf ("read: %s",packet);
	  if (packetlen != 7)
	    goto badpacket;
	  if (strncmp(packet,"PLAY",4) )
	    goto badpacket;
	  remotestage = packet[6] - '0';
	  localstage = remotestage+1;
	  if (packet[4] == '0'+ser_doomcom.consoleplayer)
	    {
	      ser_doomcom.consoleplayer ^= 1;
	      localstage = remotestage = 0;
	    }
	  oldsec = -1;
	}
      
    badpacket:
      
      time = I_GetTime() / 35;
      if (time != oldsec)
	{
	  oldsec = time;
	  sprintf (str,"PLAY%i_%i",ser_doomcom.consoleplayer,localstage);
	  Ser_WritePacket (str,strlen(str));
	}
      
    } while (remotestage < 1);
	
  //
  // flush out any extras
  //
  
  while (Ser_ReadPacket ());
}



/*
==============
=
= ModemCommand
=
==============
*/

void ModemCommand (char *str)
{
  if(!ser_active) return;         // aborted
  
  usermsg ("%s",str);
  write_buffer (str,strlen(str));
  write_buffer ("\r",1);
}

/*
==============
=
= ModemClear
=
= Clear out modem commands from previous attempts to connect
==============
*/

void ModemClear ()
{
  while(read_byte() != -1);
}



/*
==============
=
= ModemResponse
=
= Waits for OK, RING, Ser_Connect, etc
==============
*/

char response[80];

void ModemResponse (char *resp)
{
  int c;
  int respptr;
	
  if(!ser_active) return;         // it has been aborted
	
  do
    {
      respptr=0;
      do
	{
	  if(CheckForEsc())
	    {
	      Ser_Error("modem response aborted.");
	      return;
	    }
	  c = read_byte ();
	  if (c==-1)
	    continue;
	  if (c == '\n' || respptr == 79)
	    {
	      response[respptr] = 0;
	      usermsg(FC_GOLD"%s",response);
	      break;
	    }
	  if (c >= ' ')
	    {
	      response[respptr] = c;
	      respptr++;
	    }
	} while (1);
    } while (strncmp(response,resp,strlen(resp)));
}


/*
=============
=
= ReadLine
=
=============
*/

void ReadLine (FILE *f, char *dest)
{
  int c;
	
  do
    {
      c = fgetc (f);
      if (c == EOF)
	{
	  usermsg ("EOF in modem.cfg");
	  return;
	}
      if (c == '\r' || c == '\n')
	break;
      *dest++ = c;
    } while (1);
  
 *dest = 0;
}


/*
=============
=
= InitModem
=
=============
*/

void InitModem (void)
{
  ModemCommand(startup);
  ModemResponse ("OK");
}

//
// Ser_Init
//
// Read cfg from modem.cfg

void Ser_Init()
{
  FILE	*f;
  
  f = fopen ("modem.cfg","r");
  if (!f)
    {
      usermsg ("Couldn't read MODEM.CFG");
      strcpy(startup, "atz");
      strcpy(shutdown, "at z h0");
      return;
    }
  ReadLine (f, startup);
  ReadLine (f, shutdown);
  fclose (f);
}


/*
=============
=
= Dial
=
=============
*/

void Dial (void)
{
  char cmd[80];
  
  usemodem = true;
  InitModem ();
  
  if(!ser_active) return; // aborted
  
  usermsg ("");
  usermsg ("Dialing...");
  usermsg ("");
  
  sprintf (cmd,"ATDT%s",phonenum);
  
  ModemCommand(cmd);
  ModemResponse ("CONNECT");
  if(!ser_active) return; // aborted
  if (strncmp (response+8,"9600",4) )
    Ser_Error ("The Connection MUST be made at 9600\n"
	       "baud, no Error correction, no compression!\n"
	       "Check your modem initialization string!");
  ser_doomcom.consoleplayer = 1;
}


/*
=============
=
= Answer
=
=============
*/

void Answer (void)
{
  usemodem = true;
  InitModem ();
  
  if(!ser_active) return;         // aborted
  
  usermsg ("");
  usermsg ("Waiting for ring...");
  usermsg ("");
  
  ModemResponse ("RING");
  ModemCommand ("ATA");
  ModemResponse ("CONNECT");
  
  ser_doomcom.consoleplayer = 0;
}

extern void    (*netget) (void);
extern void    (*netsend) (void);

/*
=================
=
= main
=
=================
*/

void Ser_Start()
{
  c_showprompt = false;
  
  C_SetConsole();
  
  ser_active = true;
  usemodem = false;               // default usemodem to false
  
  //
  // set network characteristics
  //
	
  ser_doomcom.id = DOOMCOM_ID;
  ser_doomcom.ticdup = 1;
  ser_doomcom.extratics = 0;
  ser_doomcom.numnodes = 2;
  ser_doomcom.numplayers = 2;
  ser_doomcom.drone = 0;
  
  doomcom = &ser_doomcom;
  
  usermsg("");
  C_Seperator();
  usermsg(FC_GRAY "smmu serial mode");
  usermsg("");
  
  //
  // establish communications
  //
  
  InitPort ();
  ModemClear();

  usermsg("hit escape to abort");

  switch(connectmode)
    {
    case ANSWER:
      Answer ();
      break;
      
    case DIAL:
      Dial ();
      break;
      
    default:
      break;
    }
  
  if(!ser_active) return; // aborted
  
  Ser_Connect ();
  
  if(!ser_active) return; // aborted
  
  netdisconnect = Ser_Disconnect;
  netget = Ser_NetISR;
  netsend = Ser_NetISR;
  netgame = true;
  
  ResetNet();
  
  D_InitNetGame();
  
  ResetNet();
	
  C_SendNetData();
  
  if(!netgame) // aborted
    {
      Ser_Disconnect();
      ResetNet();
      return;
    }
  ResetNet();
  
  MN_ClearMenus();         // clear menus now connected
}

extern event_t events[MAXEVENTS];
extern int eventhead, eventtail;

int CheckForEsc()
{
  event_t *ev;
  int escape=false;
  
  I_StartTic ();
  
  for (; eventtail != eventhead; eventtail = (eventtail+1) & (MAXEVENTS-1))
    {
      ev = events + eventtail;
      if((ev->type==ev_keydown) && (ev->data1==KEYD_ESCAPE))
	escape = true;
    }
  return escape;
}

/***************************
        CONSOLE COMMANDS
 ***************************/

VARIABLE_INT(comport, NULL,             1, 4, NULL);

CONSOLE_COMMAND(nullmodem, cf_notnet | cf_buffered)
{
  connectmode = CONNECT;
  Ser_Start();
}

CONSOLE_COMMAND(dial, cf_notnet | cf_buffered)
{
  connectmode = DIAL;
  strcpy(phonenum, c_args);
  Ser_Start();
}

CONSOLE_COMMAND(answer, cf_notnet | cf_buffered)
{
  connectmode = ANSWER;
  Ser_Start();
}

CONSOLE_VARIABLE(com, comport, 0) {}

void Ser_AddCommands()
{
  C_AddCommand(nullmodem);
  C_AddCommand(dial);
  C_AddCommand(com);
  C_AddCommand(answer);
}
