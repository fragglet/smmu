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
//         AALib driver
//
//-----------------------------------------------------------------------------

#include <aalib.h>

//#include "keyboard.h"

#include "doomdef.h"
#include "d_main.h"
#include "d_event.h"
#include "v_mode.h"
#include "v_video.h"
#include "z_zone.h"

static aa_context *context;
static aa_palette palette;
static aa_renderparams *params;

extern void I_InitKeyboard();

//------------ Mouse code ----------------

static boolean noMouse;

static void InitMouse()
{

}

// get mouse movement

static void GetMouse()
{
}

//----------------------------------------

static int ScanCode2DoomCode (int a)
{
  switch(a)
    {
      //    default:   return boom_key_ascii_table[a]>8 ?
      //                 boom_key_ascii_table[a] : a+0x80;
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

  return 0;
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

// we cannot rely on aalib to correctly give us key releases
// we store the keys pressed in a buffer and turn them off
// later

static char pressed[256];
static int pressed_time[256];
static int pressed_head=0, pressed_tail=0;

static void StartTic()   // get key/mouse events
{
  unsigned char c;

  if(pressed_head != pressed_tail &&
     I_GetTime() - pressed_time[pressed_head] > 2) {
    event_t ev;

    ev.type = ev_keyup;
    ev.data1 = pressed[pressed_head];
    D_PostEvent(&ev);

    pressed_head = (pressed_head + 1) & 0xff;
  }

  while((c = aa_getevent(context, 0))) {
    event_t ev;

    if(c & AA_RELEASE) {
      ev.type = ev_keyup;
      c &= ~AA_RELEASE;
      continue;
    }
    else
      ev.type = ev_keydown;

    switch(c) {
      case AA_UP:        ev.data1 = KEYD_UPARROW; break;
      case AA_DOWN:      ev.data1 = KEYD_DOWNARROW; break;
      case AA_LEFT:      ev.data1 = KEYD_LEFTARROW; break;
      case AA_RIGHT:     ev.data1 = KEYD_RIGHTARROW; break;
      case AA_BACKSPACE: ev.data1 = KEYD_BACKSPACE; break;
      case AA_ESC:       ev.data1 = KEYD_ESCAPE; break;
      default:           ev.data1 = c; break;
    }

    D_PostEvent(&ev);

    pressed[pressed_tail] = ev.data1;
    pressed_time[pressed_tail] = I_GetTime();
    pressed_tail = (pressed_tail + 1) & 0xff;
  }
}

static void StartFrame()         // frame-syncronous events
{
}

//------------ Graphics ------------

static void FinishUpdate()       // draw screen
{
  unsigned long x, y;
  unsigned char *p = context->imagebuffer;

  for(y=0; y<aa_imgheight(context); y++)
    for(x=0; x<aa_imgwidth(context); x++) {

      int in =
	(y * SCREENWIDTH * SCREENHEIGHT) / aa_imgheight(context) +
	(x * SCREENWIDTH) / aa_imgwidth(context);
      
      *p++ = screens[0][in];
    }
      
  aa_renderpalette(context, 
		   palette, 
		   params, 
		   0, 0, 
		   aa_imgwidth(context), aa_imgheight(context));

  aa_flush(context);
}

//
// For SetPalette, instead of actually setting a palette, we instead
// use the palette to build up the palchar and palcolour tables.
// These hold the character and colour to use for each palette entry.
//

static void SetPalette(byte *pal)
{
  int i;

  for(i=0; i<256; i++)
    aa_setpalette(palette, i, pal[3 * i], pal[3 * i + 1], pal[3 * i + 2]);
}

//----------- Set mode ------------

static boolean SetMode(int i)    // set video mode
{
  hires = 0;
  return true;
}

static void UnsetMode()          // 'unset mode' - ie go to text mode
{
 // heh
}

//----------- Init ----------------

static boolean InitGraphics()    // returns true if initted ok
{
  context = aa_autoinit(&aa_defparams);

  if(!context)
    return false;

  params = aa_getrenderparams();

  aa_autoinitkbd(context, 0);

  return true;
}

static void ShutdownGraphics()   // shut down library
{
  if(context)
    aa_close(context);
}

static char *aalib_modenames[] =      // names of video modes - NULL terminated
  {
    "aalib",
    NULL
  };

viddriver_t aalib_driver =
{
  "Ascii Art Library mode!",
  "-aalib",

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
  
  aalib_modenames,
};
