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
//      DJGPP Graphics stuff
//
//-----------------------------------------------------------------------------

static const char rcsid[] = "$Id$";

#include "../z_zone.h"  /* memory allocation wrappers -- killough */

#include <stdio.h>
#include <signal.h>
#include <allegro.h>
#include <dpmi.h>
#include <sys/nearptr.h>
#include <sys/farptr.h>
#include <dos.h>
#include <go32.h>

#include "keyboard.h"

#include "../doomdef.h"
#include "../doomstat.h"

#include "../c_io.h"
#include "../c_runcmd.h"
#include "../d_main.h"

#include "../m_argv.h"
#include "../v_mode.h"
#include "../v_video.h"
#include "../w_wad.h"
#include "../z_zone.h"


///////////////////////////////////////////////////////////////////////////
//
// Input Code
//
//////////////////////////////////////////////////////////////////////////

extern void I_InitKeyboard();      // i_system.c

//
// Keyboard routines
// By Lee Killough
// Based only a little bit on Chi's v0.2 code
//

static int Alleg_ScanCode2DoomCode (int a)
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

static int Alleg_DoomCode2ScanCode (int a)
{
  static int inverse[256], cache=0;
  for (;cache<256;cache++)
    inverse[Alleg_ScanCode2DoomCode(cache)]=cache;
  return inverse[a];
}


extern int usemouse;   // killough 10/98

/////////////////////////////////////////////////////////////////////////////
//
// JOYSTICK                                                  // phares 4/3/98
//
/////////////////////////////////////////////////////////////////////////////

extern int usejoystick;
extern int joystickpresent;
extern int joy_x,joy_y;
extern int joy_b1,joy_b2,joy_b3,joy_b4;

//void poll_joystick(void);

// Alleg_JoystickEvents() gathers joystick data and creates an event_t for
// later processing by G_Responder().

static void Alleg_JoystickEvents()
{
  static int old_joy_b1, old_joy_b2, old_joy_b3, old_joy_b4;
  event_t event;

  if (!joystickpresent || !usejoystick)
    return;
  poll_joystick(); // Reads the current joystick settings

  // build joystick event
  
  event.type = ev_joystick;

  // read the button settings

  event.data1 =
    (joy_b1 ? 1 : 0) | (joy_b2 ? 2 : 0) |
    (joy_b3 ? 4 : 0) | (joy_b4 ? 8 : 0);

  // Read the x,y settings. Convert to -1 or 0 or +1.

  if (joy_x < 0)
    event.data2 = -1;
  else if (joy_x > 0)
    event.data2 = 1;
  else
    event.data2 = 0;
  if (joy_y < 0)
    event.data3 = -1;
  else if (joy_y > 0)
    event.data3 = 1;
  else
    event.data3 = 0;

  // post what you found

  D_PostEvent(&event);

  // build button events (make joystick buttons virtual keyboard keys
  // as originally suggested by lee killough in the boom suggestions file)

  if(joy_b1 != old_joy_b1)
    {
      event.type = joy_b1 ? ev_keydown : ev_keyup;
      event.data1 = KEYD_JOY1;
      D_PostEvent(&event);
      old_joy_b1 = joy_b1;
    }
  if(joy_b2 != old_joy_b2)
    {
      event.type = joy_b2 ? ev_keydown : ev_keyup;
      event.data1 = KEYD_JOY2;
      D_PostEvent(&event);
      old_joy_b2 = joy_b2;
    }
  if(joy_b3 != old_joy_b3)
    {
      event.type = joy_b3 ? ev_keydown : ev_keyup;
      event.data1 = KEYD_JOY3;
      D_PostEvent(&event);
      old_joy_b3 = joy_b3;
    }
  if(joy_b4 != old_joy_b4)
    {
      event.type = joy_b4 ? ev_keydown : ev_keyup;
      event.data1 = KEYD_JOY4;
      D_PostEvent(&event);
      old_joy_b4 = joy_b4;
    }
}


//
// Alleg_StartFrame
//
static void Alleg_StartFrame (void)
{
  Alleg_JoystickEvents(); // Obtain joystick data                 phares 4/3/98
}

/////////////////////////////////////////////////////////////////////////////
//
// END JOYSTICK                                              // phares 4/3/98
//
/////////////////////////////////////////////////////////////////////////////

// killough 3/22/98: rewritten to use interrupt-driven keyboard queue

static void Alleg_GetEvent()
{
  event_t event;
  int tail;

  while ((tail=keyboard_queue.tail) != keyboard_queue.head)
    {
      int k = keyboard_queue.queue[tail];
      int key;
      keyboard_queue.tail = (tail+1) & (KQSIZE-1);
      event.type = k & 0x80 ? ev_keyup : ev_keydown;
      event.data1 = Alleg_ScanCode2DoomCode(k & 0x7f);
      D_PostEvent(&event);
    }

  if (mousepresent!=-1 && usemouse) // killough 10/98
    {
      static int lastbuttons;
      int xmickeys,ymickeys,buttons=mouse_b;

      get_mouse_mickeys(&xmickeys,&ymickeys);
      if (xmickeys || ymickeys)
        {
          event.data1=buttons;
          event.data3=-ymickeys;
          event.data2=xmickeys;
          event.type=ev_mouse;
          D_PostEvent(&event);
        }
      if(buttons != lastbuttons)
	{
	  if((buttons & 1) != (lastbuttons & 1))
	    {
	      event.type = buttons & 1 ? ev_keydown : ev_keyup;
	      event.data1 = KEYD_MOUSE1;
	      D_PostEvent(&event);
	    }
	  if((buttons & 2) != (lastbuttons & 2))
	    {
	      event.type = buttons & 2 ? ev_keydown : ev_keyup;
	      event.data1 = KEYD_MOUSE2;
	      D_PostEvent(&event);
	    }
	  if((buttons & 4) != (lastbuttons & 4))
	    {
	      event.type = buttons & 4 ? ev_keydown : ev_keyup;
	      event.data1 = KEYD_MOUSE3;
	      D_PostEvent(&event);
	    }
          lastbuttons=buttons;
	}
    }
}

//
// Alleg_StartTic
//

static void Alleg_StartTic()
{
  Alleg_GetEvent();
}

//===========================================================================
//
// Graphics Code
//
//===========================================================================

//
// Alleg_UpdateNoBlit
//

static void Alleg_UpdateNoBlit (void)
{
}

// 1/25/98 killough: faster blit for Pentium, PPro and PII CPUs:
extern void ppro_blit(void *, size_t);
extern void pent_blit(void *, size_t);

extern void blast(void *destin, void *src);  // blits to VGA planar memory
extern void ppro_blast(void *destin, void *src);  // same but for PPro CPU

static int page_flip;     // killough 8/15/98: enables page flipping
BITMAP *screens0_bitmap;
static boolean noblit;

static boolean vga_pageflip;           // X-mode VGA pageflip
static boolean in_page_flip, in_hires, linear;
static int scroll_offset;
static unsigned long screen_base_addr;
static unsigned destscreen;
static int border_offset; 

static void Alleg_FinishUpdate(void)
{
  if(vga_pageflip)
    {
      //
      // killough 8/15/98:
      //
      // 320x200 Wait-free page-flipping for flicker-free display
      //
      // blast it to the screen
      
      destscreen += 0x4000;             // Move address up one page
      destscreen &= 0xffff;             // Reduce address mod 4 pages
      
      // Pentium Pros and above need special consideration in the
      // planar multiplexing code, to avoid partial stalls. killough
      
      if (cpu_family >= 6)
	ppro_blast((byte *) __djgpp_conventional_base
		   + 0xa0000 + destscreen, *screens);
      else
	blast((byte *) __djgpp_conventional_base  // Other CPUs, e.g. 486
	      + 0xa0000 + destscreen, *screens);
      
      // page flip 
      outportw(0x3d4, destscreen | 0x0c); 
      
      return;
    }
  else   
    {
      // hires hardware page-flipping (VBE 2.0)
      if(in_page_flip)
	scroll_offset = (scroll_offset ? 0 : 400);
      else        // killough 8/15/98: no page flipping; use other methods
	if (use_vsync && !timingdemo)
	  vsync(); // killough 2/7/98: use vsync() to prevent screen breaks.
    }

  if (linear)
    {   // 1/16/98 killough: optimization based on CPU type
      int size =
	in_hires ? SCREENWIDTH*SCREENHEIGHT*4 : SCREENWIDTH*SCREENHEIGHT;
      byte *dascreen =
	screen_base_addr +
	screen->line[scroll_offset+border_offset];
      if (cpu_family >= 6)     // PPro, PII
	ppro_blit(dascreen,size);
      else
	if (cpu_family >= 5)   // Pentium
	  pent_blit(dascreen,size);
	else                   // Others
	  memcpy(dascreen,*screens,size);
    }
  else
    {
      // Banked mode is slower. But just in case it's needed...
      blit(screens0_bitmap, screen, 0, 0, 0, scroll_offset + border_offset,
	   SCREENWIDTH << hires, SCREENHEIGHT << hires);
    }

  if (in_page_flip)  // hires hardware page-flipping (VBE 2.0)
    scroll_screen(0, scroll_offset);
}

//
// Alleg_ReadScreen
//

static void Alleg_ReadScreen(byte *scr)
{
  int size = hires ? SCREENWIDTH*SCREENHEIGHT*4 : SCREENWIDTH*SCREENHEIGHT;

  // 1/18/98 killough: optimized based on CPU type:
  if (cpu_family >= 6)     // PPro or PII
    ppro_blit(scr,size);
  else
    if (cpu_family >= 5)   // Pentium
      pent_blit(scr,size);
    else                     // Others
      memcpy(scr,*screens,size);
}

//
// killough 10/98: init disk icon
//

static BITMAP *diskflash, *old_data;

static void Alleg_InitDiskFlash(void)
{
  byte temp[32*32];

  if (diskflash)
    {
      destroy_bitmap(diskflash);
      destroy_bitmap(old_data);
    }

  //sf : disk is actually 16x15
  diskflash = create_bitmap_ex(8, 16<<hires, 15<<hires);
  old_data = create_bitmap_ex(8, 16<<hires, 15<<hires);

  V_GetBlock(0, 0, 0, 16, 15, temp);
  V_DrawPatchDirect(0, -1, 0, W_CacheLumpName(M_CheckParm("-cdrom") ?
					     "STCDROM" : "STDISK", PU_CACHE));
  V_GetBlock(0, 0, 0, 16, 15, diskflash->line[0]);
  V_DrawBlock(0, 0, 0, 16, 15, temp);
}

//
// killough 10/98: draw disk icon
//

static void Alleg_BeginRead(void)
{
  if (!disk_icon)
    return;

  blit(screen, old_data,
       (SCREENWIDTH-16) << hires,
       scroll_offset + ((SCREENHEIGHT-15)<<hires),
       0, 0, 16 << hires, 15 << hires);

  blit(diskflash, screen, 0, 0, (SCREENWIDTH-16) << hires,
       scroll_offset + ((SCREENHEIGHT-15)<<hires), 16 << hires, 15 << hires);
}

//
// killough 10/98: erase disk icon
//

static void Alleg_EndRead(void)
{
  if (!disk_icon)
    return;

  blit(old_data, screen, 0, 0, (SCREENWIDTH-16) << hires,
       scroll_offset + ((SCREENHEIGHT-15)<<hires), 16 << hires, 15 << hires);
}

static void Alleg_SetPalette(byte *palette)
{
  int i;

  if (!timingdemo)
    while (!(inportb(0x3da) & 8));

  outportb(0x3c8,0);
  for (i=0;i<256;i++)
    {
      outportb(0x3c9,gammatable[usegamma][*palette++]>>2);
      outportb(0x3c9,gammatable[usegamma][*palette++]>>2);
      outportb(0x3c9,gammatable[usegamma][*palette++]>>2);
    }
}

//===========================================================================
//
// Mode set/unset
//
//===========================================================================

//
// UnsetMode function.
// Go to text mode
//

static void Alleg_UnsetMode(void)
{
  clear(screen);
  
  // Turn off graphics mode
  set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
}

//
// killough 11/98: New routine, for setting hires and page flipping
//

// sf: now returns true if an error occurred

static boolean Alleg_SetMode(int i)
{
  set_color_depth(8);     // killough 2/7/98: use allegro set_gfx_mode
  scroll_offset = 0;
  vga_pageflip = false;

  border_offset = 0;
  
  // sf: reorganised as a switch()
  
  switch(i)
    {
      // mode 1: vga pageflipped, like original doom
      // by lee killough
      
      case 1:
	{
	  // killough 8/15/98:
	  //
	  // Page flipping code for wait-free, flicker-free display.
	  //
	  // This is the method Doom originally used, and was removed in the
	  // Linux source. It only works for 320x200. The VGA hardware is
	  // switched to planar mode, which allows 256K of VGA RAM to be
	  // addressable, and allows page flipping, split screen, and hardware
	  // panning.
	  
	  destscreen = 0;
	  
	  //
	  // VGA mode 13h
	  //
	  
	  if(set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0))
	    return false;       // init failed

	  // 
	  // turn off chain 4 and odd/even 
	  // 
	  outportb(0x3c4, 4); 
	  outportb(0x3c5, (inportb(0x3c5) & ~8) | 4); 
	  
	  // 
	  // turn off odd/even and set write mode 0 
	  // 
	  outportb(0x3ce, 5); 
	  outportb(0x3cf, inportb(0x3cf) & ~0x13);
	  
	  // 
	  // turn off chain 4 
	  // 
	  outportb(0x3ce, 6); 
	  outportb(0x3cf, inportb(0x3cf) & ~2); 
      
	  // 
	  // clear the entire buffer space, because
	  // int 10h only did 16 k / plane 
	  // 

	  if(screens[0])
	    {
	      outportw(0x3c4, 0xf02);
	      blast(0xa0000 + (byte *) __djgpp_conventional_base, screens[0]);
	    }
	  
	  // Now we do most of this stuff again for Allegro's benefit :)
	  // All that work above was just to clear the screen first.

	  if(set_gfx_mode(GFX_MODEX, 320, 200, 320, 800))
	    return false;

	  // sf: set appropriate variables
	  vga_pageflip = true;
	  hires = 0;
	}
      break;

      // 320x200 VESA
      // can be significantly faster than vga
      // 320x200 on some computers
	
      case 2:
	if(set_gfx_mode(GFX_VESA2L, 320, 200, 320, 200))
	  {
	    return false;
	  }
	else
	  {
	    hires = 0;
	    page_flip = false;
	  }
	break;
	
	// 3: hires mode 640x400
	
      case 3:
	if (set_gfx_mode(GFX_AUTODETECT, 640, 400, 0, 0))
	  {
	    return false; 
	  }
	else
	  {
	    hires = 1;
	    page_flip = false;
	  }
	break;
	
	// 4: hires mode pageflipped 640x400
	
      case 4:
	if (set_gfx_mode(GFX_AUTODETECT, 640, 400, 640, 800))
          {
	    return false;
	  }
	else
	  {
	    set_clip(screen, 0, 0, 640, 800);    // Allow full access
	    hires = 1;
	    page_flip = true;
	  }
	  
	break;

	// 5: 640x480 (640x400 with borders)
      case 5:
	if(set_gfx_mode(GFX_AUTODETECT, 640, 480, 640, 480))
	  {
	    return false;
	  }
	else
	  {
	    hires = 1;
	    page_flip = false;
	    border_offset = 40;
	  }
	break;
	
      // 0: 320x200 plain ol' VGA
      // we place this at the end with default:
	
      default:
      case 0:
	if(set_gfx_mode(GFX_AUTODETECT, 320, 200, 320, 200))
	   {
	     return false;
	   }
	else
	  {
	    hires = 0;
	    page_flip = false;
	  }
	break;
    }

  if(i != 1)
    {
      linear = is_linear_bitmap(screen);
      
      __dpmi_get_segment_base_address(screen->seg, &screen_base_addr);
      screen_base_addr -= __djgpp_base_address;
    }

  in_page_flip = page_flip;
  in_hires = hires;

  //  Alleg_InitDiskFlash();        // Initialize disk icon
  
  return true;
}

//===========================================================================
//
// Init/Shutdown
//
//===========================================================================

static void Alleg_ShutdownGraphics()
{
}

static boolean Alleg_InitGraphics(void)
{
  static int firsttime = true;

  if (!firsttime)
    return false;

  firsttime = false;

  check_cpu();    // 1/16/98 killough -- sets cpu_family based on CPU

#ifndef RANGECHECK
  asm("fninit");  // 1/16/98 killough -- prevents FPU exceptions
#endif

  timer_simulate_retrace(0);

  if (nodrawers) // killough 3/2/98: possibly avoid gfx mode
    return false;

  // init keyboard
  I_InitKeyboard();

  //
  // enter graphics mode
  //

  signal(SIGINT, SIG_IGN);  // ignore CTRL-C in graphics mode

  in_page_flip = page_flip;

  return true;              // initted ok
}

// the list of video modes is stored here in i_video.c
// the console commands to change them are in v_misc.c,
// so that all the platform-specific stuff is in here.
// v_misc.c does not care about the format of the videomode_t,
// all it asks is that it contains a text value 'description'
// which describes the mode
        
char *alleg_modenames[]=
{
  "320x200 VGA",
  "320x200 VGA (pageflipped)",
  "320x200 VESA",
  "640x400",
  "640x400 (pageflipped)",
  "640x480 (640x400 with borders)",
  NULL,
};

//=========================================================================
//
// Driver
//
//=========================================================================

viddriver_t alleg_driver = 
{
  "dos allegro",
  "-allegro",

  Alleg_InitGraphics,
  Alleg_ShutdownGraphics,

  Alleg_SetMode,
  Alleg_UnsetMode,

  Alleg_FinishUpdate,
  Alleg_SetPalette,

  Alleg_StartTic,
  Alleg_StartFrame,
  
  alleg_modenames,
};




//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.3  2000-06-19 14:57:37  fraggle
// make functions static
//
// Revision 1.2  2000/06/09 20:53:45  fraggle
// add I_StartFrame frame-syncronous stuff (joystick)
//
// Revision 1.1.1.1  2000/04/30 19:12:12  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
