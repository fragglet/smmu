/******** I_INPUT.C ********/

// sf: used to be in i_video.c
// this is all the code to interface with the controllers(keyboard, mouse,
// joystick etc). I didn't think it really fell under the 'video'
// category =).

#include "../z_zone.h"  /* memory allocation wrappers -- killough */

#include <stdio.h>

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
#include "../m_menu.h"
#include "../wi_stuff.h"
#include "../i_system.h"

extern int usemouse;   // killough 10/98

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

//
// Keyboard routines
// By Lee Killough
// Based only a little bit on Chi's v0.2 code
//

int I_ScanCode2DoomCode (int a)
{
  return 0;
}

// Automatic caching inverter, so you don't need to maintain two tables.
// By Lee Killough

int I_DoomCode2ScanCode (int a)
{
return 0;
}

// killough 3/22/98: rewritten to use interrupt-driven keyboard queue

void I_GetEvent()
{
}

/*************************
        CONSOLE COMMANDS
 *************************/

void I_Input_AddCommands()
{
}
