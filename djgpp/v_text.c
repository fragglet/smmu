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
//         Text mode driver!
//
//-----------------------------------------------------------------------------

#include <dos.h>
#include <pc.h>
#include <sys/nearptr.h>
#include <conio.h>

#include "keyboard.h"

#include "../doomdef.h"
#include "../d_main.h"
#include "../d_event.h"
#include "../v_mode.h"
#include "../v_video.h"
#include "../z_zone.h"

extern void I_InitKeyboard();

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

static unsigned char palchar[256];
static unsigned char palcolour[256];

#define TEXTWIDTH 80
#define TEXTHEIGHT (v_mode == 1 ? 50 : 25)

static void FinishUpdate()       // draw screen
{
  unsigned char writebuffer[TEXTWIDTH * TEXTHEIGHT * 2];
  int x, y;

  // build writebuffer. We map pixels in the screens[0] virtual 320x200 screen
  // to the real 80x25 (or 80x50) screen. We take the pixel value and map it
  // to a character and colour using the palchar and palcolour arrays built
  // by SetPalette
  
  for(x=0; x<TEXTWIDTH; x++)
    for(y=0; y<TEXTHEIGHT; y++)
      {
	int gx = (x * SCREENWIDTH) / TEXTWIDTH;
	int gy = (y * SCREENHEIGHT) / TEXTHEIGHT;
	int screencol = *(screens[0] + gy * SCREENWIDTH + gx); 
	
	writebuffer[(y * TEXTWIDTH + x) * 2] = palchar[screencol];
	writebuffer[(y * TEXTWIDTH + x) * 2 + 1] = palcolour[screencol];
      }

  memcpy((byte *) __djgpp_conventional_base + 0xb8000, writebuffer,
	 TEXTWIDTH * TEXTHEIGHT * 2);	       
}

//
// For SetPalette, instead of actually setting a palette, we instead
// use the palette to build up the palchar and palcolour tables.
// These hold the character and colour to use for each palette entry.
//

static void SetPalette(byte *pal)
{
  short i;

  for (i=0; i<256; i++)
    {
      // get rgb value and convert it to hls value
      
      int r = pal[3*i], g = pal[3*i + 1], b = pal[3*i + 2];
      int h, l;
      int min, max;
      
      // if r = g = b then its gray/white and we dont need
      // to calculate hue
      // (infact doing so will cause a divide by zero error)
      // TODO: make these approximate rather than exactly equal
      // eg. there isnt a hell of a lot of difference visually between
      // 128,128,128 and 128,128,129 although gray is a better
      // approximation
      
      if(r == g && g == b)
	{
	  palcolour[i] = WHITE;

	  min = max = r;        // r = g = b
	}
      else
	{
	  // calculate hue
	  // from the hue we can determine what colour this is
	  
	  if(r > g && r > b)
	    {
	      // red is greatest
	      // now check which is smallest
	      
	      min = g > b ? b : g;
	      max = r;
	      
	      h = (256 * (g - b)) / (6 * (r - min));
	    }
	  else if(g > r && g > b)
	    {
	      // green
	      
	      min = r > b ? b : r;
	      max = g;
	      
	      h = (256/3) + (256 * (b - r)) / (6 * (g - min));
	    }
	  else
	    {
	      // blue
	      
	      min = r > g ? g : r;
	      max = b;
	      
	      h = (512/3) + (256 * (r - g)) / (6 * (b - min));
	    }

	  if(h < 0)        // keep it +ve
	    h += 256; 

	  // decide on colour based on hue

	  palcolour[i] =
	    h < 10 ? RED :
	    h < 30 ? BROWN :
	    h < 65 ? YELLOW :
	    h < 108 ? GREEN :
	    h < 146 ? CYAN :
	    h < 192 ? BLUE :
	    h < 238 ? MAGENTA :
	    RED;          // loops round back to red
	}
      
      // choose which char to use based on brightness
      // of this palette entry

      l = (min + max) / 2;

      palchar[i] =
	l < 51 ? ' ' :
	l < 102 ? '°' :
	l < 153 ? '±' :
	l < 204 ? '²' :
	'Û';
    }
}

//----------- Set mode ------------

static boolean SetMode(int i)    // set video mode
{
  if(i == 1)
    textmode(C4350);
  else
    textmode(C80);
  
  hires = 0;
  
  return true;
}

static void UnsetMode()          // 'unset mode' - ie go to text mode
{
  textmode(C80);
  clrscr();
}

//----------- Init ----------------

static boolean InitGraphics()    // returns true if initted ok
{

  // init keyboard
  I_InitKeyboard();
  
  if(1)
    InitMouse();
  else
    noMouse = true;  
  
  return true;
}

static void ShutdownGraphics()   // shut down library
{
}

static char *text_modenames[] =      // names of video modes - NULL terminated
  {
    "80x25",
    "80x50",
    NULL
  };

viddriver_t text_driver =
{
  "Text mode!",
  "-textmode",

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
  
  text_modenames,
};
