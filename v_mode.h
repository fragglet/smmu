// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
// Video Mode interface
//
//-----------------------------------------------------------------------------

#ifndef __V_MODE__
#define __V_MODE__

#ifdef DJGPP
#include <allegro.h>
#endif

#include "doomtype.h"

// Set Palette
void V_SetPalette (byte* palette);

// update, blit to screen
void V_UpdateNoBlit (void);
void V_FinishUpdate (void);

// Wait for vertical retrace or pause a bit. -- in i_system.c
void I_WaitVBL(int count);

// copy screen data
void V_ReadScreen (byte* scr);

// flickering disk icon
void V_BeginRead();
void V_EndRead();

// init
void V_InitGraphics();

//------------------------------------------------------------------------
//
// Video Modes
//

extern char **modenames;          // names of available vid modes
extern int v_mode;
void V_SetMode(int i);

//----------------------------------------------------------------------------
//
// Video Drivers
//
// We create a seperate viddriver_t for each type of video interface:
// allegro, X-Window, svgalib etc. We can then choose a particular
// viddriver to use on the command line
//

typedef struct
{
  char *name;                   // name of driver
  char *cmdline;                // cmd-line parameter to specify this driver

  //----------- Init ----------------

  boolean (*InitGraphics)();    // returns true if initted ok
  void (*ShutdownGraphics)();   // shut down library

  //----------- Set mode ------------
  boolean (*SetMode)(int i);    // set video mode or open window
                                // returns true if set ok
  void (*UnsetMode)();          // 'unset mode' - ie go to text mode 
                                // or close window in X

  //------------ Graphics ------------
  void (*FinishUpdate)();       // draw screen
  void (*SetPalette)(byte *pal);
  
  //------------ Input Functions ----------
  void (*StartTic)();           // get key/mouse events
  void (*StartFrame)();         // frame-syncronous events
  
  char **modenames;             // names of video modes - NULL terminated
} viddriver_t;

//-------------------------------------------------------------------------
//
// Externals
//

extern int usemouse;
extern int usejoystick;

extern int hires;
extern int use_vsync;
extern int grabMouse;   // keep mouse inside window - for win32/X

#ifdef DJGPP
extern int page_flip;  // killough 8/15/98: enables page flipping (320x200)
extern int disk_icon;  // killough 10/98
extern int vesamode;
extern BITMAP *screens0_bitmap;   // killough 12/98
#endif

#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.2  2000-06-09 20:54:53  fraggle
// add I_StartFrame frame-syncronous stuff (joystick)
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
