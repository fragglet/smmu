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
//        Linux-specific system code
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id$";

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>

#include "../c_runcmd.h"
#include "../i_system.h"
#include "../i_sound.h"
#include "../doomstat.h"
#include "../m_misc.h"
#include "../g_game.h"
#include "../w_wad.h"
#include "../v_video.h"
#include "../m_argv.h"

ticcmd_t *I_BaseTiccmd(void)
{
  static ticcmd_t emptycmd; // killough
  return &emptycmd;
}

void I_WaitVBL(int count)
{
  //  rest((count*500)/TICRATE);
}

// Most of the following has been rewritten by Lee Killough
//
// I_GetTime
//

static volatile int realtic;

void I_timer(void)
{
}

static unsigned long lasttimereply;
static unsigned long basetime;

int I_GetTime_RealTime (void)
{
  struct timeval tv;
  struct timezone tz;
  unsigned long thistimereply;

  gettimeofday(&tv, &tz);

  thistimereply = (tv.tv_sec * TICRATE + (tv.tv_usec * TICRATE) / 1000000);

  // Fix for time problem
  if (!basetime) 
    {
      basetime = thistimereply; thistimereply = 0;
    }
  else
    thistimereply -= basetime;
  
  if (thistimereply < lasttimereply)
    thistimereply = lasttimereply;

  return (lasttimereply = thistimereply);
}

void I_SetTime(int newtime)
{
  realtic = newtime;
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

static void keyboard_handler(int scancode)
{
}

int mousepresent;
int joystickpresent;                                         // phares 4/3/98

int keyboard_installed = 0;
// static int orig_key_shifts;  // killough 3/6/98: original keyboard shift state
extern int autorun;          // Autorun state
int leds_always_off;         // Tells it not to update LEDs

void I_Shutdown(void)
{
}

void I_ResetLEDs(void)
{
}


void I_Init(void)
{
  int clock_rate = realtic_clock_rate, p;

  if ((p = M_CheckParm("-speed")) && p < myargc-1 &&
      (p = atoi(myargv[p+1])) >= 10 && p <= 1000)
    clock_rate = p;
    
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

  atexit(I_Shutdown);

  { // killough 2/21/98: avoid sound initialization if no sound & no music
    extern boolean nomusicparm, nosfxparm;
    if (!(nomusicparm && nosfxparm))
      I_InitSound();
  }
}

//
// I_Sleep
//
// From lxdoom: release time to the os
//

void I_Sleep(int time)
{
  usleep(time);
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

  M_SaveDefaults ();
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
  printf("terminated normally\n");
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
      while(1) if(getchar() == 27) return true;
    }
  return false;
}

/*************************
        CONSOLE COMMANDS
 *************************/

extern void I_Sound_AddCommands();
extern void I_Video_AddCommands();
        // add system specific commands
void I_AddCommands()
{
  I_Video_AddCommands();
  I_Sound_AddCommands();
}

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:09  fraggle
// Initial revision
//
//
//----------------------------------------------------------------------------
