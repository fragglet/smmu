// Emacs style mode select -*- C++ -*-

/******** I_INPUT.C ********/

// sf: used to be in i_video.c
// this is all the code to interface with the controllers(keyboard, mouse,
// joystick etc). I didn't think it really fell under the 'video'
// category =).

#include "../z_zone.h"  /* memory allocation wrappers -- killough */

#include <stdio.h>
#include <vgamouse.h>
#include <vgakeyboard.h>

#include "../c_runcmd.h"
#include "../doomdef.h"
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
#include "../i_system.h"

extern int usemouse;   // killough 10/98
extern boolean initialised;       // i_video.c

//
// I_StartTic
//

void I_StartTic()
{
  I_GetEvent();
}

/////////////////////////////////////////////////////////////////////////////
//
// JOYSTICK                                                  // phares 4/3/98
//
/////////////////////////////////////////////////////////////////////////////

extern int usejoystick;
extern int joystickpresent;
extern int joy_x,joy_y;
extern int joy_b1,joy_b2,joy_b3,joy_b4;

void poll_joystick(void);

// I_JoystickEvents() gathers joystick data and creates an event_t for
// later processing by G_Responder().

void I_JoystickEvents()
{
}


//
// I_StartFrame
//
void I_StartFrame (void)
{
}

/////////////////////////////////////////////////////////////////////////////
//
// END JOYSTICK                                              // phares 4/3/98
//
/////////////////////////////////////////////////////////////////////////////

void I_GetEvent()
{
  keyboard_update();
  
  mouse_update();

}

// from lxdoom:

static unsigned char scancode2doomcode[256];

int I_ScanCode2DoomCode(int c)
{
  return c;
}

int I_DoomCode2ScanCode(int c)
{
  return c;
}


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

void I_InitKeyboard()
{

  if(!initialised) vga_init();

  I_InitKBTransTable();
  
  // Start RAW keyboard handling
  keyboard_init();  
  keyboard_seteventhandler(I_KBHandler);

  mouse_seteventhandler((__mouse_handler)I_MouseEventHandler);
}

/*************************
        CONSOLE COMMANDS
 *************************/

void I_Input_AddCommands()
{
}

