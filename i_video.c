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

#include "z_zone.h"  /* memory allocation wrappers -- killough */

#include <stdio.h>
#include <signal.h>
#include <allegro.h>
#include <dpmi.h>
#include <sys/nearptr.h>
#include <sys/farptr.h>
#include <dos.h>
#include <go32.h>

#include "c_io.h"
#include "c_runcmd.h"
#include "doomstat.h"
#include "v_video.h"
#include "d_main.h"
#include "m_bbox.h"
#include "st_stuff.h"
#include "m_argv.h"
#include "w_wad.h"
#include "r_draw.h"
#include "am_map.h"
#include "m_menu.h"
#include "wi_stuff.h"
#include "i_video.h"

//
// I_UpdateNoBlit
//

void I_UpdateNoBlit (void)
{
}

// 1/25/98 killough: faster blit for Pentium, PPro and PII CPUs:
extern void ppro_blit(void *, size_t);
extern void pent_blit(void *, size_t);

extern void blast(void *destin, void *src);  // blits to VGA planar memory
extern void ppro_blast(void *destin, void *src);  // same but for PPro CPU

int use_vsync;     // killough 2/8/98: controls whether vsync is called
int page_flip;     // killough 8/15/98: enables page flipping
int hires;
int vesamode;
BITMAP *screens0_bitmap;
boolean noblit;

static int in_graphics_mode;
static int in_page_flip, in_hires, linear;
static int scroll_offset;
static unsigned long screen_base_addr;
static unsigned destscreen;

void I_FinishUpdate(void)
{
  if (noblit || !in_graphics_mode)
    return;

  if (in_page_flip)
    if (!in_hires)
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
    else       // hires hardware page-flipping (VBE 2.0)
      scroll_offset = scroll_offset ? 0 : 400;
  else        // killough 8/15/98: no page flipping; use other methods
    if (use_vsync && !timingdemo)
      vsync(); // killough 2/7/98: use vsync() to prevent screen breaks.

  if (!linear)  // Banked mode is slower. But just in case it's needed...
    blit(screens0_bitmap, screen, 0, 0, 0, scroll_offset,
	 SCREENWIDTH << hires, SCREENHEIGHT << hires);
  else
    {   // 1/16/98 killough: optimization based on CPU type
      int size =
	in_hires ? SCREENWIDTH*SCREENHEIGHT*4 : SCREENWIDTH*SCREENHEIGHT;
      byte *dascreen = screen_base_addr + screen->line[scroll_offset];
      if (cpu_family >= 6)     // PPro, PII
	ppro_blit(dascreen,size);
      else
	if (cpu_family >= 5)   // Pentium
	  pent_blit(dascreen,size);
	else                   // Others
	  memcpy(dascreen,*screens,size);
    }

  if (in_page_flip)  // hires hardware page-flipping (VBE 2.0)
    scroll_screen(0, scroll_offset);
}

//
// I_ReadScreen
//

void I_ReadScreen(byte *scr)
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

int disk_icon;

static BITMAP *diskflash, *old_data;

static void I_InitDiskFlash(void)
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

void I_BeginRead(void)
{
  if (!disk_icon || !in_graphics_mode)
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

void I_EndRead(void)
{
  if (!disk_icon || !in_graphics_mode)
    return;

  blit(old_data, screen, 0, 0, (SCREENWIDTH-16) << hires,
       scroll_offset + ((SCREENHEIGHT-15)<<hires), 16 << hires, 15 << hires);
}

void I_SetPalette(byte *palette)
{
  int i;

  if (!in_graphics_mode)             // killough 8/11/98
    return;

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

void I_ShutdownGraphics(void)
{
  if (in_graphics_mode)  // killough 10/98
    {
      clear(screen);

      // Turn off graphics mode
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);

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
  int gfx_type = GFX_AUTODETECT;

  if(vesamode && !hires && page_flip) vesamode = 0;
  if(vesamode) gfx_type = GFX_VESA2L;

  scroll_offset = 0;

  if (hires || !page_flip)
    {
      set_color_depth(8);     // killough 2/7/98: use allegro set_gfx_mode

      if (hires)
	{
	  if (page_flip)
	    if (set_gfx_mode(gfx_type, 640, 400, 640, 800))
	      {
		warn_about_changes(S_BADVID);      // Revert to no pageflipping
		page_flip = 0;
	      }
	    else
	      set_clip(screen, 0, 0, 640, 800);    // Allow full access

	  if (!page_flip && set_gfx_mode(gfx_type, 640, 400, 0, 0))
	    {
	      hires = 0;                           // Revert to lowres
	      page_flip = in_page_flip;            // Restore orig pageflipping
	      warn_about_changes(S_BADVID);
	      I_InitGraphicsMode();                // Start all over
	      return;
	    }
	}

      if (!hires)
	set_gfx_mode(gfx_type, 320, 200, 0, 0);

      linear = is_linear_bitmap(screen);

      __dpmi_get_segment_base_address(screen->seg, &screen_base_addr);
      screen_base_addr -= __djgpp_base_address;

      V_Init();
    }
  else
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

      V_Init();

      destscreen = 0;

      //
      // VGA mode 13h
      //
      
      set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0);

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
      // clear the entire buffer space, because int 10h only did 16 k / plane 
      // 
      outportw(0x3c4, 0xf02);
      blast(0xa0000 + (byte *) __djgpp_conventional_base, *screens);

      // Now we do most of this stuff again for Allegro's benefit :)
      // All that work above was just to clear the screen first.
      set_gfx_mode(GFX_MODEX, 320, 200, 320, 800);
    }

  in_graphics_mode = 1;
  in_textmode = false;
  in_page_flip = page_flip;
  in_hires = hires;

  setsizeneeded = true;

  I_InitDiskFlash();        // Initialize disk icon

  I_SetPalette(W_CacheLumpName("PLAYPAL",PU_CACHE));
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

  check_cpu();    // 1/16/98 killough -- sets cpu_family based on CPU

#ifndef RANGECHECK
  asm("fninit");  // 1/16/98 killough -- prevents FPU exceptions
#endif

  timer_simulate_retrace(0);

  if (nodrawers) // killough 3/2/98: possibly avoid gfx mode
    return;

  //
  // enter graphics mode
  //

  atexit(I_ShutdownGraphics);

  signal(SIGINT, SIG_IGN);  // ignore CTRL-C in graphics mode

  in_page_flip = page_flip;

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
        {0,0,0,"320x200 VGA"},
        {0,1,0,"320x200 VGA (pageflipped)"},
        {0,0,1,"320x200 VESA"},
        {1,0,1,"640x400 VESA"},
        {1,1,1,"640x400 VESA (pageflipped)"},
        {0,0,0, NULL}  // last one has NULL description
};

void I_SetMode(int i)
{
        static int firsttime = true;    // the first time to set mode

        hires = videomodes[i].hires;
        page_flip = videomodes[i].pageflip;
        vesamode = videomodes[i].vesa;

        if(firsttime)
                I_InitGraphicsMode();
        else
                I_ResetScreen();

        firsttime = false;
}
        
     /*****************************************************************
         check for VESA and reduce the number of modes if neccesary
                         swiped from legacy
      *****************************************************************/

// VESA information block structure
typedef struct vbeinfoblock_s
{
    unsigned char  VESASignature[4]   __attribute__ ((packed));
    unsigned short VESAVersion	      __attribute__ ((packed));
    unsigned long  OemStringPtr       __attribute__ ((packed));
    byte    Capabilities[4];
    unsigned long  VideoModePtr       __attribute__ ((packed));
    unsigned short TotalMemory	      __attribute__ ((packed));
    byte    OemSoftwareRev[2];
    byte    OemVendorNamePtr[4];
    byte    OemProductNamePtr[4];
    byte    OemProductRevPtr[4];
    byte    Reserved[222];
    byte    OemData[256];
} vbeinfoblock_t;

static vbeinfoblock_t vesainfo;

        // some #defines used
#define RM_OFFSET(addr)       (addr & 0xF)
#define RM_SEGMENT(addr)      ((addr >> 4) & 0xFFFF)
#define MASK_LINEAR(addr)     (addr & 0x000FFFFF)
#define VBEVERSION	2	// we need vesa2 or higher

void I_CheckVESA()
{
    int i;
    __dpmi_regs     regs;

    // new ugly stuff...
    for (i=0; i<sizeof(vbeinfoblock_t); i++)
       _farpokeb(_dos_ds, MASK_LINEAR(__tb)+i, 0);

    dosmemput("VBE2", 4, MASK_LINEAR(__tb));

    // see if VESA support is available
    regs.x.ax = 0x4f00;
    regs.x.di = RM_OFFSET(__tb);
    regs.x.es = RM_SEGMENT(__tb);
    __dpmi_int(0x10, &regs);

    if (regs.h.ah) goto no_vesa;

    dosmemget(MASK_LINEAR(__tb), sizeof(vbeinfoblock_t), &vesainfo);

    if (strncmp(vesainfo.VESASignature, "VESA", 4))
        goto no_vesa;

    if (vesainfo.VESAVersion < (VBEVERSION<<8))
        goto no_vesa;

        // note: does not actually check to see if any of the available
        //       vesa modes can be used in the game. Assumes all work.

    return;

    no_vesa:
    videomodes[2].description = NULL;       // cut off VESA modes

}

/************************
        CONSOLE COMMANDS
 ************************/

variable_t var_retrace =
{&use_vsync,      NULL,                 vt_int,    0,1, yesno};
variable_t var_disk =
{&disk_icon,      NULL,                 vt_int,    0,1, onoff};


command_t i_video_commands[] =
{
        {
                "v_diskicon",  ct_variable,
                0,
                &var_disk, NULL
        },
        {
                "v_retrace",   ct_variable,
                0,
                &var_retrace,V_ResetMode
        },
        {"end", ct_end}
};

void I_Video_AddCommands()
{
        C_AddCommandList(i_video_commands);
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
