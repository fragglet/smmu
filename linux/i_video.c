// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_video.c,v 1.12 1998/05/03 22:40:35 killough Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//
// DESCRIPTION:
//      DOOM graphics stuff
//
//-----------------------------------------------------------------------------

static const char rcsid[] = "$Id: i_video.c,v 1.12 1998/05/03 22:40:35 killough Exp $";

#include "../z_zone.h"  /* memory allocation wrappers -- killough */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <vga.h>
#include <vgamouse.h>
#include <vgakeyboard.h>
#include <unistd.h>

#include "../c_io.h"
#include "../c_runcmd.h"
#include "../doomstat.h"
#include "../v_video.h"
#include "../d_main.h"
#include "../m_bbox.h"
#include "../st_stuff.h"
#include "../m_argv.h"
#include "../w_wad.h"
#include "../r_draw.h"
#include "../am_map.h"
#include "../mn_engin.h"
#include "../wi_stuff.h"
#include "../i_video.h"

static enum { flip, flipped, blit } redraw_state;
static void (*blitfunc)(void* src, int dest, int w, int h, int pitch);
static enum { F_nomouse, F_mouse } mflag = F_nomouse;
int leds_always_off; // Not yet implemented
int use_vsync; // Hmm...

int hires = 0;

boolean initialised = false;
static int mode = G320x200x256;

//
// I_UpdateNoBlit
//

void I_UpdateNoBlit (void)
{
}

static int in_graphics_mode;
static int in_page_flip, in_hires, linear;
static int scroll_offset;
static unsigned long screen_base_addr;
static unsigned destscreen;

void I_FinishUpdate(void)
{
  if (noblit || !in_graphics_mode)
    return;

  switch (redraw_state) {
  case blit:
    if (use_vsync) 
      vga_waitretrace();
    (*blitfunc)(screens[0], 0, SCREENWIDTH, SCREENHEIGHT, SCREENWIDTH);
    break;
  case flipped:
    (*blitfunc)(screens[0], 0, SCREENWIDTH, SCREENHEIGHT, SCREENWIDTH);
    if (use_vsync) 
      vga_waitretrace();
    vga_setdisplaystart(0);
    redraw_state = flip;
    break;
  case flip:
    (*blitfunc)(screens[0], 1<<16, 
		 SCREENWIDTH, SCREENHEIGHT, SCREENWIDTH);
    if (use_vsync) 
      vga_waitretrace();
    vga_setdisplaystart(1<<16);
    redraw_state = flipped;
    break;
  }

}

//
// I_ReadScreen
//

void I_ReadScreen(byte *scr)
{
  int size = hires ? SCREENWIDTH*SCREENHEIGHT*4 : SCREENWIDTH*SCREENHEIGHT;

  memcpy(scr,screens[0],size);
}

// sf: blit functions

static void I_PlainBlit(void* src, int dest, int w, int h, int pitch)
{
  // sf:
  w <<= hires;
  h <<= hires;
  pitch <<= hires; 

  vga_lockvc();
  {
    if (redraw_state != blit) // We probably chose blit to avoid paging
      vga_setpage(dest >> 16); // I.e each page is 64K
    if (pitch == SCREENWIDTH) {
      //unsigned char *vga;
      int loop;
      
      // You wouldn't believe the amount of tries of raw-writing in mode-X I 
      // did before I got to this trivial solution.
      // Of course an ideal solution would be to rewrite R_DrawColumn to 
      // work in mode-X... but let's leave something for somebody else, won't
      // we?
      
      for ( loop = 0 ; loop < h ; loop++ )
	vga_drawscanline(loop, ((unsigned char *)src) + loop * w);
    }
    else {
      // IS THIS EVER CALLED?????
      register byte* destptr = vga_getgraphmem();
      do {
        memcpy(destptr, src, w);
        src = (void*)((int)src + w);
        dest += pitch;
      } while (--h);
    }
  }
  vga_unlockvc();
}


//
// killough 10/98: init disk icon
//

int disk_icon;

static void I_InitDiskFlash(void)
{
}

//
// killough 10/98: draw disk icon
//

void I_BeginRead(void)
{
}

//
// killough 10/98: erase disk icon
//

void I_EndRead(void)
{
}

void I_SetPalette(byte *pal)
{
    int buffer[256*3];
    int i;

    if (in_textmode || !vga_oktowrite())
      return;

    for(i=0; i<256; i++)
      {
	buffer[3*i] = (pal[3*i] >> 2);
	buffer[3*i+1] = (pal[3*i+1] >> 2);
	buffer[3*i+2] = (pal[3*i+2] >> 2);
      }

     vga_setpalvec(0, 256, buffer);
}

void I_ShutdownGraphics(void)
{
  if (in_graphics_mode)  // killough 10/98
    {
      in_graphics_mode = 0;
      in_textmode = true;
    }
}

extern boolean setsizeneeded;

//
// killough 11/98: New routine, for setting hires and page flipping
//

static void I_InitGraphicsMode(void)
{
  const vga_modeinfo * pinfo;

  

  //  I_InitKBTransTable();
  fprintf(stderr, "I_InitGraphics: ");

  if (!initialised) { 
    vga_init();
    initialised = true;
  }
 
  atexit(I_ShutdownGraphics); // svgalib does signal handlers itself
  
  // Start RAW keyboard handling
  // keyboard_init();
  
  //  keyboard_seteventhandler(I_KBHandler);
  
  // Start mouse handling
  if (!M_CheckParm("-nomouse")) {
    //    vga_setmousesupport(1); 
    // mflag = F_mouse;
  }
  
  if (!vga_hasmode(mode))
    I_Error("SVGALib reports video mode not available.");

  // Set default non-accelerated graphics funcs
  blitfunc = I_PlainBlit;
  redraw_state = blit; // Default

  if (!M_CheckParm("-noaccel")) {
    pinfo = vga_getmodeinfo(mode);
    
    if (pinfo->haveblit & HAVE_IMAGEBLIT) {
      blitfunc = vga_imageblt;
    }
    
    if (pinfo->flags & HAVE_RWPAGE ) {
      if (pinfo->flags & EXT_INFO_AVAILABLE) {
	if (pinfo->memory >= 2 * (1<<16)) {
	  redraw_state = flip;
	} else
	  if (devparm)
	    fprintf(stderr, "Insufficient memory for video paging. ");
      } else 
	if (devparm)
	  fprintf(stderr, "SVGALib did not return video memory size. ");
    } else 
      if (devparm)
	fprintf(stderr, "Video memory paging not available. "); 
  }

  fprintf(stderr, "Using:");

  if (redraw_state != blit)
    fprintf(stderr, " paging");

  if (use_vsync)
    fprintf(stderr, " retrace");

  if (blitfunc == vga_imageblt)
    fprintf(stderr, " accel");

  putc('\n', stderr);

  if (vga_setmode(mode))
    I_Error("Failed to set video mode");

  //  if (mflag == F_mouse)
  //  mouse_seteventhandler((__mouse_handler)I_MouseEventHandler);

  //I_InitJoystick();
  //  I_InitDiskFlash();        // Initialize disk icon
  I_SetPalette(W_CacheLumpName("PLAYPAL",PU_CACHE));
  in_textmode = false;
  in_graphics_mode = 1;
}

void I_ResetScreen(void)
{
  if (!in_graphics_mode)
    {
      setsizeneeded = true;
      V_Init();
      return;
    }

  I_ShutdownGraphics();     // Switch out of old graphics mode

  I_InitGraphicsMode();     // Switch to new graphics mode

  if (automapactive)
    AM_Start();             // Reset automap dimensions

  ST_Start();               // Reset palette

  if (gamestate == GS_INTERMISSION)
    {
      WI_DrawBackground();
      V_CopyRect(0, 0, 1, SCREENWIDTH, SCREENHEIGHT, 0, 0, 0);
    }

  Z_CheckHeap();
}

void I_InitGraphics(void)
{
  static int firsttime = true;

  if (!firsttime)
    return;

  firsttime = false;

  if (nodrawers) // killough 3/2/98: possibly avoid gfx mode
    return;

  //
  // enter graphics mode
  //

  I_InitGraphicsMode();

  //  in_page_flip = page_flip;

  V_ResetMode();

  Z_CheckHeap();
}

        // the list of video modes is stored here in i_video.c
        // the console commands to change them are in v_misc.c,
        // so that all the platform-specific stuff is in here.
        // v_misc.c does not care about the format of the videomode_t,
        // all it asks is that it contains a text value 'description'
        // which describes the mode
        
videomode_t videomodes[]=
{
        {"320x200 Linux"},
        {NULL}  // last one has NULL description
};

void I_SetMode(int i)
{
        static int firsttime = true;    // the first time to set mode

        if(firsttime)
                I_InitGraphicsMode();
        else
                I_ResetScreen();

        firsttime = false;
}
        
/************************
        CONSOLE COMMANDS
 ************************/

void I_Video_AddCommands()
{
}

//----------------------------------------------------------------------------
//
// $Log: i_video.c,v $
// Revision 1.12  1998/05/03  22:40:35  killough
// beautification
//
// Revision 1.11  1998/04/05  00:50:53  phares
// Joystick support, Main Menu re-ordering
//
// Revision 1.10  1998/03/23  03:16:10  killough
// Change to use interrupt-driver keyboard IO
//
// Revision 1.9  1998/03/09  07:13:35  killough
// Allow CTRL-BRK during game init
//
// Revision 1.8  1998/03/02  11:32:22  killough
// Add pentium blit case, make -nodraw work totally
//
// Revision 1.7  1998/02/23  04:29:09  killough
// BLIT tuning
//
// Revision 1.6  1998/02/09  03:01:20  killough
// Add vsync for flicker-free blits
//
// Revision 1.5  1998/02/03  01:33:01  stan
// Moved __djgpp_nearptr_enable() call from I_video.c to i_main.c
//
// Revision 1.4  1998/02/02  13:33:30  killough
// Add support for -noblit
//
// Revision 1.3  1998/01/26  19:23:31  phares
// First rev with no ^Ms
//
// Revision 1.2  1998/01/26  05:59:14  killough
// New PPro blit routine
//
// Revision 1.1.1.1  1998/01/19  14:02:50  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
