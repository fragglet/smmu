// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
// SVGA graphics
//
// From lxdoom i_video_svga.c
// Plus parts from the mbf dos i_video.c
//
//----------------------------------------------------------------------------

#include "../config.h" /* read config, do we have libsvga? */

#ifdef HAVE_LIBVGA /* define to allow compile w/out svga support */

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
#include "../d_main.h"
#include "../doomdef.h"
#include "../doomstat.h"
#include "../m_argv.h"
#include "../w_wad.h"
#include "../v_mode.h"

static int basewidth, baseheight;

static boolean initialised = false;
//static boolean usemouse = false;

//////////////////////////////////////////////////////////////////////////////
//
// Input Code
//

extern int usemouse;   // killough 10/98

// currently no joystick support.
// does it matter?

static void SVGA_GetEvent()
{
  keyboard_update();
  if(usemouse)
    mouse_update();
}

//
// SVGA_StartTic
//

void SVGA_StartTic()
{
  SVGA_GetEvent();
}

int SVGA_DoomCode2ScanCode(int c)
{
  return c;
}

int SVGA_ScanCode2DoomCode(int c)
{
  return c;
}

// from lxdoom:

static unsigned char scancode2doomcode[256];

static inline void SVGA_InitKBTransTable(void)
{
  { // First do rows of standard keys
#define KEYROWS 5
    const char keyrow[KEYROWS][20]= { { "1234567890" }, { "qwertyuiop" }, 
				      { "asdfghjkl;'" }, { "\\zxcvbnm,./`" }, 
   { KEYD_F1, KEYD_F2, KEYD_F3, KEYD_F4, KEYD_F5, 
   KEYD_F6, KEYD_F7, KEYD_F8, KEYD_F9, KEYD_F10, 0} };
    const int scancode4keyrow[KEYROWS] = { SCANCODE_1, SCANCODE_Q,
					   SCANCODE_A, SCANCODE_BACKSLASH,
					   SCANCODE_F1 };
    int i = KEYROWS;
    while (i--) {
      int j = strlen(keyrow[i]);
      while (j--) {
	scancode2doomcode[(scancode4keyrow[i] + j) & 0xff] = 
	  keyrow[i][j];
      }
    }
#undef KEYROWS
  }
  // Now the rest
  {
#define SETKEY(A,B) scancode2doomcode[SCANCODE_ ## A] = KEYD_ ## B
    SETKEY(BREAK_ALTERNATIVE,PAUSE); // BREAK_ALTERNATIVE is the pause key
    // (note that BREAK is printscreen) - rain    
    SETKEY(CURSORRIGHT,RIGHTARROW);
    SETKEY(CURSORUP,UPARROW);
    SETKEY(CURSORDOWN,DOWNARROW);
    SETKEY(CURSORLEFT,LEFTARROW);
    SETKEY(CURSORBLOCKRIGHT,RIGHTARROW);
    SETKEY(CURSORBLOCKUP,UPARROW);
    SETKEY(CURSORBLOCKDOWN,DOWNARROW);
    SETKEY(CURSORBLOCKLEFT,LEFTARROW);
    SETKEY(EQUAL,EQUALS);
    SETKEY(SPACE,SPACEBAR);
    SETKEY(LEFTALT,LALT);
    SETKEY(CAPSLOCK,CAPSLOCK);
    SETKEY(GRAVE, CONSOLE); //sf

#ifdef LCTRL
    SETKEY(LEFTCONTROL,LCTRL);
#else
    SETKEY(LEFTCONTROL,RCTRL);
#endif
#ifdef LSHIFT
    SETKEY(LEFTSHIFT,LSHIFT);
#else
    SETKEY(LEFTSHIFT,RSHIFT);
#endif
    SETKEY(RIGHTALT,RALT);
    SETKEY(RIGHTCONTROL,RCTRL);
    SETKEY(RIGHTSHIFT,RSHIFT);
    SETKEY(KEYPADENTER,ENTER);
#undef SETKEY
#define SETKEY(A) scancode2doomcode[SCANCODE_ ## A] = KEYD_ ## A
    SETKEY(ESCAPE);
    SETKEY(TAB);
    SETKEY(BACKSPACE);
    SETKEY(MINUS);
    SETKEY(INSERT);
    SETKEY(HOME);
    SETKEY(END);
    SETKEY(PAGEUP);
    SETKEY(PAGEDOWN);
    SETKEY(BACKSPACE);
    SETKEY(F11);
    SETKEY(F12);
    SETKEY(ENTER);
#undef SETKEY
  }
}

event_t ev;

static void SVGA_KBHandler(int scancode, int press)
{

  ev.type = (press == KEY_EVENTPRESS) ? ev_keydown : ev_keyup;
  if (scancode < 256) {
    ev.data1= scancode2doomcode[scancode];
    
    D_PostEvent(&ev);
  }
}


static void SVGA_MouseEventHandler(int button, int dx, int dy, 
				int dz, int rdx, int rdy, int rdz)
{
  ev.type = ev_mouse;
  ev.data1= ((button & MOUSE_LEFTBUTTON) ? 1 : 0)
    | ((button & MOUSE_MIDDLEBUTTON) ? 2 : 0)
    | ((button & MOUSE_RIGHTBUTTON)  ? 4 : 0);
  ev.data2 = dx << 2; ev.data3 = -(dy << 2);

  D_PostEvent(&ev);
}

void SVGA_ShutdownKeyboard()
{
  // shut down keyboard
  keyboard_close();
  system("stty sane");

  // shut down mouse
  
  if(usemouse)
    vga_setmousesupport(0);
}

static void SVGA_InitKeyboard()
{  
  // Start RAW keyboard handling
  keyboard_init();  
  keyboard_seteventhandler(SVGA_KBHandler);

  SVGA_InitKBTransTable();

  atexit(SVGA_ShutdownKeyboard);

  // Start mouse handling
  if (!M_CheckParm("-nomouse")) 
    {
      vga_setmousesupport(1); 
      usemouse = true;
      mouse_seteventhandler((__mouse_handler)SVGA_MouseEventHandler);
    }
}

//
// SVGA_StartFrame
//

void SVGA_StartFrame()
{
}

/////////////////////////////////////////////////////////////////////////////
//
// Graphics Code
//

static enum { flip, flipped, blit } redraw_state;
static void (*blitfunc)(void* src, int dest, int w, int h, int pitch);

static int mode = G320x200x256;
static int blit_offset;              // blitting offset into vid memory 

//
// SVGA_UpdateNoBlit
//

void SVGA_UpdateNoBlit (void)
{
}

//  static int in_page_flip, in_hires, linear;
//  static int scroll_offset;
//  static unsigned long screen_base_addr;
//  static unsigned destscreen;

void SVGA_FinishUpdate(void)
{
  switch (redraw_state)
    {
      case blit:
	if (use_vsync) 
	  vga_waitretrace();
	(*blitfunc)(screens[0], blit_offset, basewidth, baseheight, basewidth);
	break;
      case flipped:
	(*blitfunc)(screens[0], blit_offset, basewidth, baseheight, basewidth);
	if (use_vsync) 
	  vga_waitretrace();
	vga_setdisplaystart(0);
	redraw_state = flip;
	break;
      case flip:
	(*blitfunc)(screens[0], (1<<16) + blit_offset, 
		    basewidth, baseheight, basewidth);
	if (use_vsync) 
	  vga_waitretrace();
	vga_setdisplaystart(1<<16);
	redraw_state = flipped;
	break;
    }
  
}

//
// SVGA_ReadScreen
//

void SVGA_ReadScreen(byte *scr)
{
  int size = hires ? basewidth*baseheight*4 : basewidth*baseheight;

  memcpy(scr,screens[0],size);
}

// sf: blit functions

static void SVGA_PlainBlit(void* src, int dest, int w, int h, int pitch)
{
  vga_lockvc();
  
  if (redraw_state != blit) // We probably chose blit to avoid paging
    vga_setpage(dest >> 16); // I.e each page is 64K

  if (pitch == basewidth) 
    {
      //unsigned char *vga;
      register int y;
      register char *destptr;
      
      // You wouldn't believe the amount of tries of raw-writing in mode-X I 
      // did before I got to this trivial solution.
      // Of course an ideal solution would be to rewrite R_DrawColumn to 
      // work in mode-X... but let's leave something for somebody else, won't
      // we?
      
      // sf: made loop a bit more efficient

      dest %= (1 << 16);         // sf: offset it down
      h += dest;

      for ( y = dest, destptr = src ; y < h ; y++, destptr += w )
	  vga_drawscanline(y, destptr);
    }
  else
    {
      // IS THIS EVER CALLED?????
      register byte* destptr = vga_getgraphmem();
      do 
	{
	  memcpy(destptr, src, w);
	  src = (void*)((int)src + w);
	  dest += pitch;
	} while (--h);
    }

  vga_unlockvc();
}


//
// killough 10/98: init disk icon
//

static void SVGA_InitDiskFlash(void)
{
}

//
// killough 10/98: draw disk icon
//

void SVGA_BeginRead(void)
{
}

//
// killough 10/98: erase disk icon
//

void SVGA_EndRead(void)
{
}

void SVGA_SetPalette(byte *pal)
{
  int buffer[256*3];
  int i;
  
  if (!vga_oktowrite())
    return;
  

  for(i=0; i<256; i++)
    {
      buffer[3*i] = (gamma_xlate[pal[3*i]] >> 2);
      buffer[3*i+1] = (gamma_xlate[pal[3*i+1]] >> 2);
      buffer[3*i+2] = (gamma_xlate[pal[3*i+2]] >> 2);
    }
  
  vga_setpalvec(0, 256, buffer);
}

void SVGA_ShutdownGraphics(void)
{
  SVGA_ShutdownKeyboard();
}

//------------------------------------------------------------------------
//
// Set Graphics mode
//

static boolean SVGA_InitGraphicsMode(void)
{
  const vga_modeinfo * pinfo;
  
  use_vsync = 1;

  // Set default non-accelerated graphics funcs
  blitfunc = SVGA_PlainBlit;
  redraw_state = blit; // Default

  if (!M_CheckParm("-noaccel")) 
    {
      pinfo = vga_getmodeinfo(mode);
      
      if (pinfo->haveblit & HAVE_IMAGEBLIT) 
	{
	  blitfunc = vga_imageblt;
	}
      
      if (pinfo->flags & HAVE_RWPAGE ) 
	{
	  if (pinfo->flags & EXT_INFO_AVAILABLE) 
	    {
	      if (pinfo->memory >= 2 * (1<<16)) 
		{
		  redraw_state = flip;
		}
	      else
		if (devparm)
		  fprintf(stderr, "Insufficient memory for video paging. ");
	    } 
	  else 
	    if (devparm)
	      fprintf(stderr, "SVGALib did not return video memory size. ");
	} 
      else 
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
    return false;         // could not set mode

  basewidth = SCREENWIDTH << hires;
  baseheight = SCREENHEIGHT << hires;

  // sf: if we are using 640x480 for hires, blit it 40 pixels down from
  // the top so that it is centered properly

  blit_offset = mode == G640x480x256 ? (40) : 0;

  return true;                 // set ok
}

boolean SVGA_InitGraphics(void)
{
  static int firsttime = true;

  if (!firsttime)
    return false;
  firsttime = false;

  vga_init();

  // try the lowest mode and see if we can set it

  if (!vga_hasmode(G320x200x256))
    {
      printf("SVGALib reports video mode not available.");
      return false;           // init failed
    }

  SVGA_InitKeyboard();

  return true;                // initted ok
}

// unset mode ie. go to text mode

void SVGA_UnsetMode()
{
  vga_setmode(TEXT);
}

// the list of video modes is stored here in i_video.c
// the console commands to change them are in v_misc.c,
// so that all the platform-specific stuff is in here.
// v_misc.c does not care about the format of the videomode_t,
// all it asks is that it contains a text value 'description'
// which describes the mode

char *svga_modenames[]=
{
  "320x200 SVGA",
  "640x400 SVGA",
  NULL,  // last one has NULL description
};

boolean SVGA_SetMode(int i)
{
  if(i)
    {
      mode = G640x480x256;
      hires = 1;
    }
  else
    {
      mode = G320x200x256;
      hires = 0;
    }

  //
  // enter graphics mode
  //
      
  if(!SVGA_InitGraphicsMode())
    return false;

  v_mode = i;

  return true;
}
        
viddriver_t svga_driver =
  {
    "svgalib",
    "-svga",

    SVGA_InitGraphics,
    SVGA_ShutdownGraphics,

    SVGA_SetMode,
    SVGA_UnsetMode,

    SVGA_FinishUpdate,
    SVGA_SetPalette,

    SVGA_StartTic,
    SVGA_StartFrame,
    svga_modenames,
  };

#endif /* #ifdef HAVE_LIBVGA */

//-----------------------------------------------------------------------------
//
// $Log$
// Revision 1.5  2001-01-13 14:50:51  fraggle
// include config.h to check for appropriate libraries
//
// Revision 1.4  2001/01/13 02:28:23  fraggle
// changed library #defines to standard HAVE_LIBxyz
// for autoconfing
//
// Revision 1.3  2000/06/20 21:09:50  fraggle
// tweak gamma correction stuff
//
// Revision 1.2  2000/06/09 20:53:50  fraggle
// add I_StartFrame frame-syncronous stuff (joystick)
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
// 
//-----------------------------------------------------------------------------
