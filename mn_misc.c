// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Menu - misc functions
//
// Pop up alert/question messages
// Miscellaneous stuff
//
// By Simon Howard
//
//----------------------------------------------------------------------------

#include <stdarg.h>

#include "doomstat.h"
#include "s_sound.h"
#include "v_misc.h"
#include "z_zone.h"

#include "mn_engin.h"
#include "mn_misc.h"

/******************
  POP-UP MESSAGES
  *****************/

char popup_message[128];
boolean popup_message_active = false;
char *popup_message_command;            // console command to run
enum
{
  popup_alert,
  popup_question
} popup_message_type;

static void WriteCentredText(char *message)
{
  static char *tempbuf = NULL;
  static int allocedsize=0;
  char *rover, *addrover;
  int x, y;

  // mmmm.. fast
  if(strlen(message) > allocedsize)
    {
      if(tempbuf)
	tempbuf = Z_Realloc(tempbuf, strlen(message)+3, PU_STATIC, 0);
      else
	tempbuf = Z_Malloc(strlen(message)+3, PU_STATIC, 0);
    }

  y = (SCREENHEIGHT - V_StringHeight(popup_message)) / 2;
  addrover = tempbuf;
  rover = message;

  while(*rover)
    {
      if(*rover == '\n')
	{
	  *addrover = NULL;  // end string
	  x = (SCREENWIDTH - V_StringWidth(tempbuf)) / 2;
	  V_WriteText(tempbuf, x, y);
	  addrover = tempbuf;  // reset addrover
	  y += 7; // next line
	}
      else      // add next char
	{
	  *addrover = *rover;
	  addrover++;
	}
      rover++;
    }

  // dont forget the last line.. prob. not \n terminated

  *addrover = NULL;
  x = (SCREENWIDTH - V_StringWidth(tempbuf)) / 2;
  V_WriteText(tempbuf, x, y);
}

void MN_PopupDrawer()
{
  // if this has been called, assume we are
  // drawing a message :)

  WriteCentredText(popup_message);
}

boolean MN_PopupResponder(event_t *ev)
{
  if(ev->type != ev_keydown) return false;

  switch(popup_message_type)
    {
    case popup_alert:
      if(ev->data1 == KEYD_ENTER)
	{
	  // kill message
	  popup_message_active = false;
	  S_StartSound(NULL, sfx_swtchx);
	}
      break;

    case popup_question:
      if(tolower(ev->data1) == 'y')     // yes!
	{
	  // run command and kill message
	  menuactive = false; // kill menu
	  cmdtype = c_menu;
	  C_RunTextCmd(popup_message_command);
	  S_StartSound(NULL, sfx_pistol);
	  popup_message_active = false; 
	}
      if(tolower(ev->data1) == 'n' || ev->data1 == KEYD_ESCAPE)     // no!
	{
	  // kill message
	  menuactive = false; // kill menu
	  S_StartSound(NULL, sfx_swtchx);
	  popup_message_active = false;
	}
      break;
      
    default:
      break;
    }
  
  return true; // always eatkey
}

// alert message
// -- just press enter

void MN_Alert(char *message, ...)
{
  va_list args;

  MN_ActivateMenu();

  popup_message_active = true;
  popup_message_type = popup_alert;
  
  va_start(args, message);
  vsprintf(popup_message, message, args);
  va_end(args);
}

// question message
// console command will be run if user responds with 'y'

void MN_Question(char *message, char *command)
{
  MN_ActivateMenu();
  popup_message_active = true;
  strncpy(popup_message, message, 126);
  popup_message_type = popup_question;
  popup_message_command = command;
}

// popup messages used in the game


