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

#include <signal.h>
#include <windows.h>

#include "../doomdef.h"
#include "../m_argv.h"
#include "../d_main.h"
#include "../i_system.h"

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

// proff 07/04/98: Added for CYGWIN32 compatibility
#if defined(_WIN32) && !defined(_MSC_VER)
#define MAX_ARGC 128
static char    *argvbuf[MAX_ARGC];
static char    *cmdLineBuffer;

char ** commandLineToArgv(int *pArgc)
{
    char    *p, *pEnd;
    int     argc = 0;

    cmdLineBuffer=GetCommandLine();
    if (cmdLineBuffer == NULL)
        I_Error("No commandline!");

    p = cmdLineBuffer;
    pEnd = p + strlen(cmdLineBuffer);
    if (pEnd >= &cmdLineBuffer[1022]) pEnd = &cmdLineBuffer[1022];

    while (1) {
        while ((*p == ' ') && (p<pEnd)) p++;
        if (p >= pEnd) break;

        if (*p=='\"')
        {
            p++;
            if (p >= pEnd) break;
            argvbuf[argc++] = p;
            if (argc >= MAX_ARGC) break;
            while (*p && (*p != ' ') && (*p != '\"')) p++;
            if ((*p == ' ') | (*p == '\"')) *p++ = 0;
        }
        else
        {
            argvbuf[argc++] = p;
            if (argc >= MAX_ARGC) break;
            while (*p && (*p != ' ')) p++;
            if (*p == ' ') *p++ = 0;
        }
    }

    *pArgc = argc;
    return argvbuf;
}

#endif

HINSTANCE main_hInstance;

extern int Init_ConsoleWin(HINSTANCE hInstance);
extern int Init_Win(HINSTANCE hInstance);

void I_Quit(void);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		   PSTR szCmdLine, int iCmdShow)

{
  myargv = commandLineToArgv(&myargc);

  main_hInstance = hInstance;  // save it for v_win32.c
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

  Z_Init();                  // 1/18/98 killough: start up memory stuff first
  atexit(I_Quit);
  signal(SIGSEGV, handler);
  signal(SIGTERM, handler);
  signal(SIGILL,  handler);
  signal(SIGFPE,  handler);
  signal(SIGILL,  handler);
  signal(SIGINT,  handler);  // killough 3/6/98: allow CTRL-BRK during init
  signal(SIGABRT, handler);
 
  D_DoomMain ();

  return 0;
}


//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-06-19 15:00:23  fraggle
// cygwin support
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
