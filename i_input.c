/******** I_INPUT.C ********/

// sf: used to be in i_video.c
// this is all the code to interface with the controllers(keyboard, mouse,
// joystick etc). I didn't think it really fell under the 'video'
// category =).

#include "z_zone.h"  /* memory allocation wrappers -- killough */

#include <stdio.h>
#include <signal.h>
#include <allegro.h>
#include <dpmi.h>
#include <sys/nearptr.h>
#include <dos.h>

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
#include "i_system.h"

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
  event_t event;

  if (!joystickpresent || !usejoystick)
    return;
  poll_joystick(); // Reads the current joystick settings
  event.type = ev_joystick;
  event.data1 = 0;

  // read the button settings

  if (joy_b1)
    event.data1 |= 1;
  if (joy_b2)
    event.data1 |= 2;
  if (joy_b3)
    event.data1 |= 4;
  if (joy_b4)
    event.data1 |= 8;

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
}


//
// I_StartFrame
//
void I_StartFrame (void)
{
  I_JoystickEvents(); // Obtain joystick data                 phares 4/3/98
}

/////////////////////////////////////////////////////////////////////////////
//
// END JOYSTICK                                              // phares 4/3/98
//
/////////////////////////////////////////////////////////////////////////////

//
// Keyboard routines
// By Lee Killough
// Based only a little bit on Chi's v0.2 code
//

int I_ScanCode2DoomCode (int a)
{
  switch (a)
    {
    default:   return key_ascii_table[a]>8 ? key_ascii_table[a] : a+0x80;
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

int I_DoomCode2ScanCode (int a)
{
  static int inverse[256], cache;
  for (;cache<256;cache++)
    inverse[I_ScanCode2DoomCode(cache)]=cache;
  return inverse[a];
}

// killough 3/22/98: rewritten to use interrupt-driven keyboard queue

void I_GetEvent()
{
  extern int usemouse;   // killough 10/98
  event_t event;
  int tail;

  while ((tail=keyboard_queue.tail) != keyboard_queue.head)
    {
      int k = keyboard_queue.queue[tail];
      keyboard_queue.tail = (tail+1) & (KQSIZE-1);
      event.type = k & 0x80 ? ev_keyup : ev_keydown;
      event.data1 = I_ScanCode2DoomCode(k & 0x7f);
      D_PostEvent(&event);
    }

  if (mousepresent!=-1 && usemouse) // killough 10/98
    {
      static int lastbuttons;
      int xmickeys,ymickeys,buttons=mouse_b;
      get_mouse_mickeys(&xmickeys,&ymickeys);
      if (xmickeys || ymickeys || buttons!=lastbuttons)
        {
          lastbuttons=buttons;
          event.data1=buttons;
          event.data3=-ymickeys;
          event.data2=xmickeys;
          event.type=ev_mouse;
          D_PostEvent(&event);
        }
    }
}


