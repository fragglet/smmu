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
//      Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id$";

#include "../doomdef.h"
#include "../m_argv.h"
#include "../d_main.h"
#include "../i_system.h"

#include <signal.h>
#include <sys/nearptr.h>  /* needed for __djgpp_nearptr_enable() -- stan */
#include <dpmi.h>
#include <allegro.h>

// cleanup handling -- killough:
static void handler(int s)
{
  char buf[2048];

  signal(s,SIG_IGN);  // Ignore future instances of this signal.

  strcpy(buf,
         s==SIGSEGV ? "Segmentation Violation" :
         s==SIGINT  ? "Interrupted by User" :
         s==SIGILL  ? "Illegal Instruction" :
         s==SIGFPE  ? "Floating Point Exception" :
         s==SIGTERM ? "Killed" : "Terminated by signal");

  // If corrupted memory could cause crash, dump memory
  // allocation history, which points out probable causes

  if (s==SIGSEGV || s==SIGILL || s==SIGFPE)
    Z_DumpHistory(buf);

  I_Error(buf);
}

void I_Quit(void);

int main(int argc, char **argv)
{
  myargc = argc;
  myargv = argv;

  /*
     killough 1/98:

     This fixes some problems with exit handling
     during abnormal situations.

     The old code called I_Quit() to end program,
     while now I_Quit() is installed as an exit
     handler and exit() is called to exit, either
     normally or abnormally. Seg faults are caught
     and the error handler is used, to prevent
     being left in graphics mode or having very
     loud SFX noise because the sound card is
     left in an unstable state.
  */

  allegro_init();
  Z_Init();                  // 1/18/98 killough: start up memory stuff first
  atexit(I_Quit);
  signal(SIGSEGV, handler);
  signal(SIGTERM, handler);
  signal(SIGILL,  handler);
  signal(SIGFPE,  handler);
  signal(SIGILL,  handler);
  signal(SIGINT,  handler);  // killough 3/6/98: allow CTRL-BRK during init
  signal(SIGABRT, handler);

  // 2/2/98 Stan
  // Must call this here.  It's required by both netgames and i_video.c.

  if (__djgpp_nearptr_enable())  //handle nearptr now
    D_DoomMain ();
  else
    printf ("Failed trying to allocate DOS near pointers.\n");

  return 0;
}


//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.2  2000-06-04 17:19:22  fraggle
// easier reliable-packet send interface
//
// Revision 1.1.1.1  2000/04/30 19:12:12  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
