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
// SMMU Dedicated Server
//
// We can build a seperate dedicated server program if we need to: nodes
// connect to the server as they would any normal server.
//
//---------------------------------------------------------------------------

#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <stdarg.h>
#include <unistd.h>

#include "doomdef.h"
#include "net_modl.h"
#include "sv_serv.h"

//===========================================================================
//
// Standard services provided by Normal doom:
//
//===========================================================================

//---------------------------------------------------------------------------
//
// Time functions
//

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

int (*I_GetTime)() = I_GetTime_RealTime;                           // killough

//---------------------------------------------------------------------------
//
// Error
//

void I_Error(const char *error, ...) // killough 3/20/98: add const
{
  va_list argptr;
  va_start(argptr,error);
  vprintf(error,argptr);
  va_end(argptr);
  
  exit(0);
}

//============================================================================
//
// Signal Handler
//
// Deal with error signals and exit() properly
//
//============================================================================

void sig_handler(int s)
{
  signal(s,SIG_IGN);  // Ignore future instances of this signal.

  printf("Shutting down: %s\n",
	 s==SIGSEGV ? "Segmentation Violation" :
	 s==SIGINT  ? "Interrupted by User" :
	 s==SIGILL  ? "Illegal Instruction" :
	 s==SIGFPE  ? "Floating Point Exception" :
	 s==SIGTERM ? "Killed" : "Terminated by signal");
  
  exit(-1);
}

// init sig handler
// set handler to detect all signals likely to occur

static void InitSig()
{
  signal(SIGSEGV, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGILL,  sig_handler);
  signal(SIGFPE,  sig_handler);
  signal(SIGILL,  sig_handler);
  signal(SIGINT,  sig_handler);
  signal(SIGABRT, sig_handler);
}


//===========================================================================
//
// Main
//
//===========================================================================

int extratics = 2;

void DED_Shutdown()
{
  // shutdown server
  SV_Shutdown();
}

void main()
{
  printf("\nSMMU dedicated server\n");

  InitSig();          // set up signal handler

  // start up server

  atexit(DED_Shutdown);
  SV_Init();
  SV_StartServer();

  // add tcp/ip module

  UDP_InitLibrary();

  if(!SV_AddModule(&udp)) 
    {
      I_Error("couldnt init tcp/ip\n");
    }

  // main loop

  while(true)
    {
      // call server update

      SV_Update();         

      // return some time to the os

      usleep(10000);
    }
}

