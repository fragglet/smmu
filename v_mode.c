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
// Generalised Video
//
// We create a "driver" for each type of video: x-windows, svga etc.
// and put all the relevant functions into a viddriver_t structure.
// This way we can choose which type of display we want simply from
// command lines, without the need for seperate executables.
// Of course, we also include #defines for people without a particular
// driver
//
//----------------------------------------------------------------------------

#include "c_io.h"
#include "c_runcmd.h"
#include "am_map.h"
#include "m_argv.h"
#include "doomdef.h"
#include "doomstat.h"
#include "r_main.h"
#include "v_mode.h"
#include "v_video.h"
#include "w_wad.h"
#include "wi_stuff.h"
#include "z_zone.h"

//============================================================================
//
// Video Code
//
//============================================================================

void V_InitGraphics();

// video 'driver'
// different video drivers can be selected for different
// environments/libraries: eg, svgalib, X
// some oses only have one driver, eg dos

viddriver_t *viddriver = NULL;
static boolean initted_driver = false;
boolean in_graphics_mode = false;
int current_mode = -1;         // current video mode

int usemouse = 1;
int usejoystick;

int leds_always_off;
int use_vsync;
int grabMouse;
int disk_icon;

int hires = 0;

void V_StartTic()
{
  if(in_graphics_mode)
    viddriver->StartTic();
}

void V_StartFrame()
{
  if(in_graphics_mode)
    viddriver->StartFrame();
}

//
// I_UpdateNoBlit
//

void V_UpdateNoBlit (void)
{
}

void V_FinishUpdate(void)
{
  if (noblit)
    return;

  if(in_graphics_mode)
    viddriver->FinishUpdate();
}

#ifdef DJGPP

// 1/25/98 killough: faster blit for Pentium, PPro and PII CPUs:
extern void ppro_blit(void *, size_t);
extern void pent_blit(void *, size_t);

#endif

void V_ReadScreen(byte *scr)
{
  int size = hires ? SCREENWIDTH*SCREENHEIGHT*4 : SCREENWIDTH*SCREENHEIGHT;

#ifdef DJGPP /* in DJGPP use fast asm blitting functions */

  // 1/18/98 killough: optimized based on CPU type:
  if (cpu_family >= 6)     // PPro or PII
    ppro_blit(scr, size);
  else
    if (cpu_family >= 5)   // Pentium
      pent_blit(scr, size);
    else                     // Others
      memcpy(scr, screens[0], size);

#else
  
  // plain ol' memcpy  
  memcpy(scr, screens[0], size);
  
#endif
}

//
// killough 10/98: init disk icon
//

static void V_InitDiskFlash(void)
{
}

//
// killough 10/98: draw disk icon
//

void V_BeginRead(void)
{
}

//
// killough 10/98: erase disk icon
//

void V_EndRead(void)
{
}

void V_SetPalette(byte *pal)
{
  if(viddriver && in_graphics_mode)
    viddriver->SetPalette(pal);
}

//===========================================================================
//
// Mode Setting
//
//===========================================================================

char **modenames;          // names of video modes
int v_mode = 0;

// find number of video modes

static int NumModes()
{
  int i=0;

  while(modenames[i])
    i++;

  return i;
}

//------------------------------------------------------------------------
//
// V_SetMode
//
// Change video mode
//

void V_SetMode(int i)
{
  // check driver is initted

  if(!initted_driver)
    return;

  // check for invalid mode

  if(i >= NumModes() || i < 0)
    {
      C_Printf("invalid mode %i", i);

      if(in_graphics_mode)      // if initting, make sure we go to gfx mode
	return;
    }

  // go to text mode (or close window in X or Win32)
      
  if(in_graphics_mode)
    {
      if(current_mode == i)   // check if already in mode
	return;
      
      viddriver->UnsetMode(); // go to text mode
    }
  
  // set mode
  
  if(!viddriver->SetMode(i))
    {
      // mode set failed
      // try mode 0

      if(viddriver->SetMode(current_mode))
	i = current_mode;
      else
	I_Error("I_SetMode: couldnt reset mode");
    }
  
  // reset various modules.
  // but only if we are changing modes and not
  // setting for the first time

  V_Init();                     // v_init is always called

  if(in_graphics_mode)
    {
      setsizeneeded = true;

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

  // in video mode now

  in_graphics_mode = true;

  // change v_mode to new mode

  current_mode = v_mode = i;
  
  // set correct palette

  V_SetPalette(W_CacheLumpName("PLAYPAL",PU_CACHE));
}

//--------------------------------------------------------------------------
//
// v_mode Console Variable
//

VARIABLE_INT(v_mode, NULL,              0, 10, NULL);
CONSOLE_VARIABLE(v_mode, v_mode, cf_buffered)
{
  V_SetMode(v_mode);
}

//--------------------------------------------------------------------------
//
// List Video Modes
//

CONSOLE_COMMAND(v_modelist, 0)
{
  int i;
  
  C_Printf(FC_GRAY "video modes:\n" FC_RED);
  
  while(modenames[i])
    {
      C_Printf("%i: %s\n", i, modenames[i]);
      i++;
    }
}

unsigned char *gamma_xlate;      // gamma translation table
int usegamma;

CONSOLE_INT(gamma, usegamma, NULL,                        0, 4, NULL, 0)
{
  // change to new gamma val
  gamma_xlate = gammatable[usegamma];
  V_SetPalette (W_CacheLumpName ("PLAYPAL",PU_CACHE));
}


//===========================================================================
//
// Library Init/Shutdown
//
//===========================================================================

//---------------------------------------------------------------------------
//
// I_ShutdownGraphics
//
// Called on exit
//

void V_ShutdownGraphics(void)
{
  if (in_graphics_mode)  // killough 10/98
    {
      viddriver->UnsetMode();           // go to text mode
      in_graphics_mode = false;
    }

  if(initted_driver)
    viddriver->ShutdownGraphics();
}

//---------------------------------------------------------------------------
//
// I_InitGraphics
//
// Decide on graphics 'driver' and set function ptrs.
// Then call viddriver->InitGraphics
//

#ifdef DJGPP  
extern viddriver_t alleg_driver;
extern viddriver_t vga_driver;
extern viddriver_t text_driver;
#endif
#ifdef HAVE_LIBX11    /* X Window */
extern viddriver_t xwin_driver;
#endif
#ifdef HAVE_LIBVGA    /* svgalib */
extern viddriver_t svga_driver;
#endif
#ifdef _WIN32         /* windows driver/cygwin */
extern viddriver_t win32_driver;
#endif
#ifdef HAVE_LIBAA     /* aalib */
extern viddriver_t aalib_driver;
#endif

static viddriver_t *drivers[] =
{
#ifdef DJGPP
  &alleg_driver,
  &vga_driver,
  &text_driver,
#endif
#ifdef HAVE_LIBX11
  &xwin_driver,
#endif
#ifdef HAVE_LIBVGA
  &svga_driver,
#endif
#ifdef _WIN32
  &win32_driver,
#endif
#ifdef HAVE_LIBAA
  &aalib_driver,
#endif
};

const int num_drivers = sizeof(drivers) / sizeof(*drivers);

void V_InitGraphics(void)
{
  int i;

  if(devparm)   // we wait if in devparm so the user can see the messages
    {
      printf("devparm: press a key..\n");
      getchar();
    }

  // sf: gamma correction
  
  gamma_xlate = gammatable[usegamma];
  
  // check for cmd-line parameter override

  for(i=0; i<num_drivers; i++)
    if(M_CheckParm(drivers[i]->cmdline))
       {
	 printf("V_InitGraphics: Trying driver: '%s'\n", drivers[i]->name);
	 if(drivers[i]->InitGraphics())
	   {
	     viddriver = drivers[i];
	     break;
	   }
	 else
	   I_Error("V_InitGraphics: driver '%s' failed to init\n",
		   drivers[i]->name);
       }
  
  // go through drivers and try each one in turn until a
  // working one is found
  
  if(!viddriver)
    {
      for(i=0; i<num_drivers; i++)
	{
	  printf("V_InitGraphics: Trying driver: '%s'\n", drivers[i]->name);
	  
	  // try driver, leave loop if it works
	  
	  if(drivers[i]->InitGraphics())
	    {
	      viddriver = drivers[i];
	      break;
	    }
	}
      
      if(!viddriver)
	{
	  I_Error("V_InitGraphics: No working video driver!");
	}
    }
  
  // found a working driver

  initted_driver = true;

  // set modes
  modenames = viddriver->modenames;

  atexit(V_ShutdownGraphics);

  V_SetMode(v_mode);

  // startup in console

  gamestate = GS_CONSOLE;
  current_height = SCREENHEIGHT;
}

//==========================================================================
//
// Console Commands
//
//==========================================================================

CONSOLE_BOOLEAN(v_retrace, use_vsync, NULL, yesno, 0) {}
CONSOLE_BOOLEAN(v_diskicon, disk_icon, NULL, onoff, 0) {}
CONSOLE_BOOLEAN(v_grabmouse, grabMouse, NULL, yesno, 0) {}

CONSOLE_BOOLEAN(use_mouse, usemouse, NULL, yesno, 0) {}
CONSOLE_BOOLEAN(use_joystick, usejoystick, NULL, yesno, 0) {}

void V_Mode_AddCommands()
{
  C_AddCommand(use_mouse);
  C_AddCommand(use_joystick);
  C_AddCommand(gamma);

  C_AddCommand(v_mode);
  C_AddCommand(v_modelist);

  C_AddCommand(v_retrace);
  C_AddCommand(v_diskicon);
  C_AddCommand(v_grabmouse);
}

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.8  2001-01-13 02:29:46  fraggle
// changed library #defines to standard HAVE_LIBxyz
// for autoconfing
//
// Revision 1.7  2000/08/16 12:16:01  fraggle
// text mode driver
//
// Revision 1.6  2000/07/29 22:40:18  fraggle
// twiddle gamma correction stuff
//
// Revision 1.5  2000/06/22 18:29:38  fraggle
// VGA Mode driver for Zokum
//
// Revision 1.4  2000/06/20 21:09:40  fraggle
// tweak gamma correction stuff
//
// Revision 1.3  2000/06/19 14:58:55  fraggle
// cygwin (win32) support
//
// Revision 1.2  2000/06/09 20:53:30  fraggle
// add I_StartFrame frame-syncronous stuff (joystick)
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
