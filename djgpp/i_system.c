// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// DESCRIPTION:
//     DJGPP specific system code
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id$";

#include <stdio.h>

#include <allegro.h>
#include <stdarg.h>
#include <gppconio.h>
#include <sys/nearptr.h>
#include <signal.h>

#include "keyboard.h"

#include "../c_runcmd.h"
#include "../i_system.h"
#include "../i_sound.h"
#include "../doomstat.h"
#include "../g_bind.h"
#include "../g_game.h"
#include "../m_argv.h"
#include "../m_misc.h"
#include "../v_mode.h"
#include "../v_video.h"
#include "../w_wad.h"

ticcmd_t *I_BaseTiccmd(void)
{
  static ticcmd_t emptycmd; // killough
  return &emptycmd;
}

void I_WaitVBL(int count)
{
  rest((count*500)/TICRATE);
}

// Most of the following has been rewritten by Lee Killough
//
// I_GetTime
//

static volatile int realtic;

void I_timer(void)
{
  realtic++;
}
END_OF_FUNCTION(I_timer);

int  I_GetTime_RealTime (void)
{
  return realtic;
}

void I_SetTime(int newtime)
{
  asm("cli");
  realtic = newtime;
  asm("sti");
}

        //sf: made a #define, changed to 16
#define CLOCK_BITS 16

// killough 4/13/98: Make clock rate adjustable by scale factor
int realtic_clock_rate = 100;
static long long I_GetTime_Scale = 1<<CLOCK_BITS;
int I_GetTime_Scaled(void)
{
  return (long long) realtic * I_GetTime_Scale >> CLOCK_BITS;
}

static int  I_GetTime_FastDemo(void)
{
  static int fasttic;
  return fasttic++;
}

static int I_GetTime_Error()
{
  I_Error("Error: GetTime() used before initialization");
  return 0;
}

int (*I_GetTime)() = I_GetTime_Error;                           // killough

// killough 3/21/98: Add keyboard queue

struct keyboard_queue_s keyboard_queue;

void keyboard_handler(int scancode)
{
  keyboard_queue.queue[keyboard_queue.head++] = scancode;
  keyboard_queue.head &= KQSIZE-1;
}
static END_OF_FUNCTION(keyboard_handler);

int mousepresent;
int joystickpresent;                                         // phares 4/3/98

static int keyboard_installed = 0;
static int orig_key_shifts;  // killough 3/6/98: original keyboard shift state
extern int autorun;          // Autorun state
int leds_always_off;         // Tells it not to update LEDs

void I_Shutdown(void)
{
  if (mousepresent!=-1)
    remove_mouse();

  // killough 3/6/98: restore keyboard shift state
  key_shifts = orig_key_shifts;

  remove_keyboard();
  keyboard_installed = false;

  remove_timer();
}

void I_ResetLEDs(void)
{
  // Either keep the keyboard LEDs off all the time, or update them
  // right now, and in the future, with respect to key_shifts flag.
  //
  // killough 10/98: moved to here

  boom_set_leds(leds_always_off ? 0 : -1);
}

void I_InitKeyboard()
{
  // killough 3/21/98: Install handler to handle interrupt-driven keyboard IO
  LOCK_VARIABLE(keyboard_queue);
  LOCK_FUNCTION(keyboard_handler);
  boom_keyboard_lowlevel_callback = keyboard_handler;

  boom_install_keyboard();
  keyboard_installed = true;

  // killough 3/6/98: save keyboard state, initialize shift state and LEDs:

  orig_key_shifts = key_shifts;  // save keyboard state

  key_shifts = 0;        // turn off all shifts by default

  if (autorun)  // if autorun is on initially, turn on any corresponding shifts
    switch (key_autorun)
      {
      case KEYD_CAPSLOCK:
        key_shifts = KB_CAPSLOCK_FLAG;
        break;
      case KEYD_NUMLOCK:
        key_shifts = KB_NUMLOCK_FLAG;
        break;
      case KEYD_SCROLLLOCK:
        key_shifts = KB_SCROLOCK_FLAG;
        break;
      }

  I_ResetLEDs();
}

extern void Ser_ReadModemCfg();   // net_ser.c

void I_Init(void)
{
  extern int key_autorun;
  int clock_rate = realtic_clock_rate, p;

  if ((p = M_CheckParm("-speed")) && p < myargc-1 &&
      (p = atoi(myargv[p+1])) >= 10 && p <= 1000)
    clock_rate = p;
    
  //init timer
  LOCK_VARIABLE(realtic);
  LOCK_FUNCTION(I_timer);
  install_timer();
  install_int_ex(I_timer,BPS_TO_TIMER(TICRATE));

  // killough 4/14/98: Adjustable speedup based on realtic_clock_rate
  if (fastdemo)
    I_GetTime = I_GetTime_FastDemo;
  else
    if (clock_rate != 100)
      {
        I_GetTime_Scale = ((long long) clock_rate << CLOCK_BITS) / 100;
        I_GetTime = I_GetTime_Scaled;
      }
    else
      I_GetTime = I_GetTime_RealTime;

  // killough 3/6/98: end of keyboard / autorun state changes

  //init the mouse
  mousepresent=install_mouse();
  if (mousepresent!=-1)
    show_mouse(NULL);

  // phares 4/3/98:
  // Init the joystick
  // For now, we'll require that joystick data is present in allegro.cfg.
  // The ASETUP program can be used to obtain the joystick data.

  if (load_joystick_data(NULL) == 0)
    joystickpresent = true;
  else
    joystickpresent = false;

  atexit(I_Shutdown);

  if(0)
  { // killough 2/21/98: avoid sound initialization if no sound & no music
    extern boolean nomusicparm, nosfxparm;
    if (!(nomusicparm && nosfxparm))
      I_InitSound();
  }

  // get modem cfg

  Ser_ReadModemCfg();
}

//
// I_Quit
//

static char errmsg[2048];    // buffer of error message -- killough

static int has_exited;

void I_Quit (void)
{
  has_exited = 1;   /* Prevent infinitely recursive exits -- killough */

  if (demorecording)
    G_CheckDemoStatus();

        // sf : rearrange this so the errmsg doesn't get messed up
  if (*errmsg)
    puts(errmsg);   // killough 8/8/98
  else
    I_EndDoom();

  G_SaveDefaults ();
}

//
// I_Error
//

void I_Error(const char *error, ...) // killough 3/20/98: add const
{
  if (!*errmsg)   // ignore all but the first message -- killough
    {
      va_list argptr;
      va_start(argptr,error);
      vsprintf(errmsg,error,argptr);
      va_end(argptr);
    }

  if (!has_exited)    // If it hasn't exited yet, exit now -- killough
    {
      has_exited=1;   // Prevent infinitely recursive exits -- killough
      exit(-1);
    }
}

// killough 2/22/98: Add support for ENDBOOM, which is PC-specific
// killough 8/1/98: change back to ENDOOM

void I_EndDoom(void)
{
  int lump;

  if (lumpinfo && (lump = W_CheckNumForName("ENDOOM")) != -1) // killough 10/98
    {  // killough 8/19/98: simplify
      memcpy(0xb8000 + (byte *) __djgpp_conventional_base,
	     W_CacheLumpNum(lump, PU_CACHE), 0xf00);
      gotoxy(1,24);
    }
}

void I_Sleep(int time)
{
  return;
}

        // check for ESC button pressed, regardless of keyboard handler
int I_CheckAbort()
{
  if(keyboard_installed)
    {
      event_t *ev;
      
      V_StartTic ();       // build events
      
      // use the keyboard handler
      for ( ; eventtail != eventhead ; eventtail = (++eventtail)&(MAXEVENTS-1) )
	{ 
	  ev = &events[eventtail]; 
	  if (ev->type == ev_keydown && ev->data1 == key_escape)      // phares
	    {                   // abort
	      return true;
	    }
	}
    }
  else
    {
      // check normal keyboard handler
      while(kbhit()) if(getch() == 27) return true;
    }
  return false;
}


// from legacy:
// we need to know if windows is loaded - libsocket uses
// a winsock back door for networking

boolean I_DetectWin95 (void)
{
  __dpmi_regs r;
  
  r.x.ax = 0x160a;        // Get Windows Version
  __dpmi_int(0x2f, &r);
  
  if(r.x.ax || r.h.bh < 4)    // Not windows or earlier than Win95
    {
      return false;
    }
  else
    {
      return true;
    }
}

//==========================================================================
//
// Console Commands
//
//==========================================================================

CONSOLE_INT(i_gamespeed, realtic_clock_rate, NULL,  0, 500, NULL, 0)
{
  if (realtic_clock_rate != 100)
    {
      I_GetTime_Scale = ((long long) realtic_clock_rate << CLOCK_BITS) / 100;
      I_GetTime = I_GetTime_Scaled;
    }
  else
    I_GetTime = I_GetTime_RealTime;
  
  ResetNet();         // reset the timers and stuff
}

CONSOLE_BOOLEAN(i_ledsoff, leds_always_off, NULL, yesno, 0)
{
  I_ResetLEDs();
}

extern void I_Sound_AddCommands();
extern void Ser_AddCommands();

// add system specific commands
void I_AddCommands()
{
  C_AddCommand(i_ledsoff);
  C_AddCommand(i_gamespeed);
  
  I_Sound_AddCommands();
  Ser_AddCommands();
}

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.2  2000-06-09 20:55:14  fraggle
// fix keyboard
//
// Revision 1.1.1.1  2000/04/30 19:12:12  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
