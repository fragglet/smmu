// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// SVGA graphics
//
// From lxdoom i_video_svga.c
// Plus parts from the mbf dos i_video.c
//
//----------------------------------------------------------------------------


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

static boolean initialised = false;

/************************* Input functions ***********************************/

extern int usemouse;   // killough 10/98

// currently no joystick support.
// does it matter?

static void I_GetEvent()
{
  keyboard_update();
  
  mouse_update();
}

//
// I_StartTic
//

void I_StartTic()
{
  I_GetEvent();
}

int I_DoomCode2ScanCode(int c)
{
  return c;
}

int I_ScanCode2DoomCode(int c)
{
  return c;
}

// from lxdoom:

static unsigned char scancode2doomcode[256];

static inline void I_InitKBTransTable(void)
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

static void I_KBHandler(int scancode, int press)
{

  ev.type = (press == KEY_EVENTPRESS) ? ev_keydown : ev_keyup;
  if (scancode < 256) {
    ev.data1= scancode2doomcode[scancode];
    
    D_PostEvent(&ev);
  }
}


static void I_MouseEventHandler(int button, int dx, int dy, 
				int dz, int rdx, int rdy, int rdz)
{
  ev.type = ev_mouse;
  ev.data1= ((button & MOUSE_LEFTBUTTON) ? 1 : 0)
    | ((button & MOUSE_MIDDLEBUTTON) ? 2 : 0)
    | ((button & MOUSE_RIGHTBUTTON)  ? 4 : 0);
  ev.data2 = dx << 2; ev.data3 = -(dy << 2);

  D_PostEvent(&ev);
}

static void I_InitKeyboard()
{
  I_InitKBTransTable();
  
  // Start RAW keyboard handling
  keyboard_init();  
  keyboard_seteventhandler(I_KBHandler);

  mouse_seteventhandler((__mouse_handler)I_MouseEventHandler);
}

//
// I_StartFrame
//

void I_StartFrame()
{
}

/************************* Graphics code ********************************/

static enum { flip, flipped, blit } redraw_state;
static void (*blitfunc)(void* src, int dest, int w, int h, int pitch);
// static enum { F_nomouse, F_mouse } mflag = F_nomouse;
int leds_always_off; // Not yet implemented
int use_vsync; // Hmm...
int hires = 0;

static int mode = G320x200x256;

//
// I_UpdateNoBlit
//

void I_UpdateNoBlit (void)
{
}

static int in_graphics_mode;
//  static int in_page_flip, in_hires, linear;
//  static int scroll_offset;
//  static unsigned long screen_base_addr;
//  static unsigned destscreen;

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
      buffer[3*i] = (gammatable[usegamma][pal[3*i]] >> 2);
      buffer[3*i+1] = (gammatable[usegamma][pal[3*i+1]] >> 2);
      buffer[3*i+2] = (gammatable[usegamma][pal[3*i+2]] >> 2);
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
    I_Error("Failed to set video mode 320x200x256");

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

  I_InitKeyboard();

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
    {NULL},  // last one has NULL description
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

VARIABLE_BOOLEAN(usemouse, NULL,         yesno);
CONSOLE_VARIABLE(use_mouse, usemouse,    0) {}

void I_Video_AddCommands()
{
  C_AddCommand(use_mouse);
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
