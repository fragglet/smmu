// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
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
// DESCRIPTION:
//      Plain VGA Driver - fix for Zokum and anyone else having
//      problems with Allegro
//
//-----------------------------------------------------------------------------

#include <dos.h>
#include <pc.h>
#include <sys/nearptr.h>

#include "keyboard.h"

#include "../doomdef.h"
#include "../d_event.h"
#include "../v_mode.h"
#include "../v_video.h"
#include "../z_zone.h"

static char *scrptr;          // screen memory location
static union REGS r;

//------------ Mouse code ----------------

static boolean noMouse;

static void InitMouse()
{
  r.x.ax = 0;         // init mouse
  int86(0x33, &r, &r);

  noMouse = r.x.ax == 0;
}

// get mouse movement

static void GetMouse()
{
  event_t ev;
  int x, y;
  int buttons, dbuttons;
  static int oldbuttons;
  
  r.x.ax = 0xb;
  int86(0x33, &r, &r);

  x = (signed short)r.x.cx;
  y = (signed short)r.x.dx;

  if(x || y)
    {
      ev.type = ev_mouse;
      ev.data1 = 0;
      ev.data2 = x;
      ev.data3 = -y;
  
      D_PostEvent(&ev);
    }

  // check buttons

  r.x.ax = 3;
  int86(0x33, &r, &r);
  
  buttons = r.x.bx;

  dbuttons = buttons ^ oldbuttons;

  if(dbuttons & 1)
    {
      ev.type = (buttons & 1) ? ev_keydown : ev_keyup;
      ev.data1 = KEYD_MOUSE1;
      D_PostEvent(&ev);
    }
  if(dbuttons & 2)
    {
      ev.type = (buttons & 2) ? ev_keydown : ev_keyup;
      ev.data1 = KEYD_MOUSE2;
      D_PostEvent(&ev);
    }
  if(dbuttons & 4)
    {
      ev.type = (buttons & 4) ? ev_keydown : ev_keyup;
      ev.data1 = KEYD_MOUSE3;
      D_PostEvent(&ev);
    }

  oldbuttons = buttons;
}

//----------------------------------------

static int ScanCode2DoomCode (int a)
{
  switch (a)
    {
    default:   return boom_key_ascii_table[a]>8 ?
                 boom_key_ascii_table[a] : a+0x80;
    case 0x7b: return KEYD_PAUSE;
    case 0x0e: return KEYD_BACKSPACE;
    case 0x48: return KEYD_UPARROW;
    case 0x4d: return KEYD_RIGHTARROW;
    case 0x50: return KEYD_DOWNARROW;
    case 0x4b: return KEYD_LEFTARROW;
    case 0x38: return KEYD_LALT;
    case 0x79: return KEYD_RALT;
    case 0x1d:
    case 0x78: return KEYD_RCTRL;
    case 0x36:
    case 0x2a: return KEYD_RSHIFT;
  }
}

// Automatic caching inverter, so you don't need to maintain two tables.
// By Lee Killough

static int DoomCode2ScanCode (int a)
{
  static int inverse[256], cache=0;
  for (;cache<256;cache++)
    inverse[ScanCode2DoomCode(cache)]=cache;
  return inverse[a];
}

//------------ Input Functions ----------

static void StartTic()   // get key/mouse events
{
  event_t event;
  int tail;

  // keyboard events
  
  while ((tail=keyboard_queue.tail) != keyboard_queue.head)
    {
      int k = keyboard_queue.queue[tail];
      int key;
      keyboard_queue.tail = (tail+1) & (KQSIZE-1);
      event.type = k & 0x80 ? ev_keyup : ev_keydown;
      event.data1 = ScanCode2DoomCode(k & 0x7f);
      D_PostEvent(&event);
    }

  if(!noMouse)
    GetMouse();
}

static void StartFrame()         // frame-syncronous events
{
}

//------------ Graphics ------------

static void FinishUpdate()       // draw screen
{
  memcpy(scrptr, screens[0], 64000);
}

static void SetPalette(byte *pal)
{
  short i;
 
  outp(0x3c8,0);
  
  for (i=0; i<256*3; i++)
    {
      outp(0x3c9, (*pal++) >> 2);
    }
}

//----------- Set mode ------------

static boolean SetMode(int i)    // set video mode
{
  hires = 0;
  
  r.h.ah=0;
  r.h.al=0x13;
  int86(0x10, &r, &r);

  return true;
}

static void UnsetMode()          // 'unset mode' - ie go to text mode
{
  r.h.ah=0;
  r.h.al=0x03;
  int86(0x10, &r, &r);
}

//----------- Init ----------------

static boolean InitGraphics()    // returns true if initted ok
{
  // set scrptr
  scrptr = (char*) (__djgpp_conventional_base + 0xa0000);

  // init keyboard
  I_InitKeyboard();

  if(1) //use_mouse)
    InitMouse();
  else
    noMouse = true;
      
  
  return true;
}

static void ShutdownGraphics()   // shut down library
{
}

static char *vga_modenames[] =      // names of video modes - NULL terminated
  {
    "320x200",
    NULL
  };

viddriver_t vga_driver =
{
  "Plain VGA",
  "-plainvga",

  //----------- Init ----------------

  InitGraphics,
  ShutdownGraphics,
  
  //----------- Set mode ------------
  SetMode,
  UnsetMode,

  //------------ Graphics ------------
  FinishUpdate,
  SetPalette,
  
  //------------ Input Functions ----------
  StartTic,
  StartFrame,
  
  vga_modenames,
};
