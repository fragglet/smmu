// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
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
//	DOOM graphics stuff for X11, UNIX.
//
//      Based on the X code from xdoom, although modified quite a bit
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#ifdef XWIN /* define to allow compile w/out x-win support */

#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifndef NOSHM
#include <X11/extensions/XShm.h>
#endif

// Had to dig up XShm.c for this one.
// It is in the libXext, but not in the X headers.
//#if defined(LINUX)
int XShmGetEventBase( Display* dpy );
//#endif

#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <errno.h>
#include <signal.h>

#if defined(SCOOS5) || defined(SCOUW2) || defined(SCOUW7)
#include "../strcmp.h"
#endif

#include "../c_runcmd.h"
#include "../doomstat.h"
#include "../doomdef.h"
#include "../d_main.h"
#include "../i_system.h"
#include "../m_argv.h"
#include "../v_mode.h"
#include "../v_video.h"
#include "../w_wad.h"

// sf: hires

static int basewidth;                 // SCREENWIDTH << hires
static int baseheight;                // SCREENHEIGHT << hires

extern boolean	showkey;

static Display *X_display=NULL;
static Window X_mainWindow;
static boolean window_open = false;
static Colormap X_cmap;
static Visual *X_visual;
static GC X_gc;
static XEvent X_event;
static int X_screen;
static XVisualInfo X_visualinfo;
static XImage  *image;
static int X_width;
static int X_height;
static Atom X_deletewin;

#ifndef NOSHM
// MIT SHared Memory extension.
static boolean doShm;

static XShmSegmentInfo X_shminfo;
static int X_shmeventtype;
#endif

// Mouse handling.
static boolean Mousegrabbed = false;

// Blocky mode,
// replace each 320x200 pixel with multiply*multiply pixels.
// According to Dave Taylor, it still is a bonehead thing
// to use ....
static int multiply=1;

// X visual mode
static int	x_depth=1;
static int	x_bpp=1;
static int	x_pseudo=1;
static unsigned short* x_colormap2 = 0;
static unsigned char* x_colormap3 = 0;
static unsigned long* x_colormap4 = 0;
static unsigned long x_red_mask = 0;
static unsigned long x_green_mask = 0;
static unsigned long x_blue_mask = 0;
static unsigned char x_red_offset = 0;
static unsigned char x_green_offset = 0;
static unsigned char x_blue_offset = 0;

// sf: whether the mouse should be grabbed at this particular moment
// do not grab mouse in console or menu or when the game is paused

#define grabnow \
 ( grabMouse && !paused && !menuactive && gamestate != GS_CONSOLE)

// in linux, all doom code/scan code conversion done in here

int XWin_DoomCode2ScanCode(int i)
{
  return i;
}

int XWin_ScanCode2DoomCode(int i)
{
  return i;
}

//
//  Translates the key currently in X_event
//
static int xlatekey(void)
{
  
  int rc;
  
  switch(rc = XKeycodeToKeysym(X_display, X_event.xkey.keycode, 0))
    {
    case XK_Left:
    case XK_KP_Left:	rc = KEYD_LEFTARROW;	break;
      
    case XK_Right:
    case XK_KP_Right:	rc = KEYD_RIGHTARROW;	break;
      
    case XK_Down:
    case XK_KP_Down:	rc = KEYD_DOWNARROW;	break;
      
    case XK_Up:
    case XK_KP_Up:	rc = KEYD_UPARROW;	break;
      
    case XK_Escape:	rc = KEYD_ESCAPE;	break;
    case XK_Return:	rc = KEYD_ENTER;		break;
    case XK_Tab:	rc = KEYD_TAB;		break;
    case XK_F1:	        rc = KEYD_F1;		break;
    case XK_F2:     	rc = KEYD_F2;		break;
    case XK_F3:	        rc = KEYD_F3;		break;
    case XK_F4:        	rc = KEYD_F4;		break;
    case XK_F5:	        rc = KEYD_F5;		break;
    case XK_F6:	        rc = KEYD_F6;		break;
    case XK_F7:	        rc = KEYD_F7;		break;
    case XK_F8:     	rc = KEYD_F8;		break;
    case XK_F9: 	rc = KEYD_F9;		break;
    case XK_F10:	rc = KEYD_F10;		break;
    case XK_F11:	rc = KEYD_F11;		break;
    case XK_F12:	rc = KEYD_F12;		break;
      
    case XK_BackSpace:
    case XK_Delete:	rc = KEYD_BACKSPACE;	break;
      
    case XK_Pause:	rc = KEYD_PAUSE;		break;
      
    case XK_KP_Equal:
    case XK_KP_Add:
    case XK_equal:	rc = KEYD_EQUALS;	break;
      
    case XK_KP_Subtract:
    case XK_minus:	rc = KEYD_MINUS;		break;
      
    case XK_Shift_L:
    case XK_Shift_R:
      rc = KEYD_RSHIFT;
      break;

    case XK_Page_Up:   rc = KEYD_PAGEUP;      break;
    case XK_Page_Down: rc = KEYD_PAGEDOWN;    break;

    case XK_Caps_Lock:
      rc = KEYD_CAPSLOCK;
      break;
      
    case XK_Control_L:
    case XK_Control_R:
      rc = KEYD_RCTRL;
      break;
      
    case XK_Alt_L:
    case XK_Meta_L:
    case XK_Alt_R:
    case XK_Meta_R:
      rc = KEYD_RALT;
      break;
      
    default:
      if (rc >= XK_space && rc <= XK_asciitilde)
	rc = rc - XK_space + ' ';
      if (rc >= 'A' && rc <= 'Z')
	rc = rc - 'A' + 'a';
      break;
    }
  
  //      if (showkey)
  //        fprintf(stdout,"Key: %d\n", rc);
  
  return rc;
}

void XWin_ShutdownGraphics(void)
{
#ifndef NOSHM
  // Release shared memory
  shmctl(X_shminfo.shmid, IPC_RMID, 0);
#endif

  // Paranoia.
  image->data = NULL;

  // disconnect from X server

  XCloseDisplay(X_display);
}



//
// XWin_StartFrame
//
void XWin_StartFrame(void)
{
  /* frame syncronous IO operations not needed for X11 */
}

static int	lastmousex = 0;
static int	lastmousey = 0;
boolean		shmFinished;

void XWin_GetEvent(void)
{
  event_t event;
  
  // put event-grabbing stuff in here
  XNextEvent(X_display, &X_event);
  switch (X_event.type)
    {
    case KeyPress:
      event.type = ev_keydown;
      event.data1 = xlatekey();
      D_PostEvent(&event);
      // fprintf(stderr, "k");
      break;
      
    case KeyRelease:
      event.type = ev_keyup;
      event.data1 = xlatekey();
      D_PostEvent(&event);
      // fprintf(stderr, "ku");
      break;
      
    case ButtonPress:
      event.type = ev_mouse;
      event.data1 =
	(X_event.xbutton.state & Button1Mask ? 1 : 0)
	| (X_event.xbutton.state & Button2Mask ? 2 : 0)
	| (X_event.xbutton.state & Button3Mask ? 4 : 0)
	| (X_event.xbutton.button == Button1 ? 1 : 0)
	| (X_event.xbutton.button == Button2 ? 2 : 0)
	| (X_event.xbutton.button == Button3 ? 4 : 0);
      event.data2 = event.data3 = 0;
      D_PostEvent(&event);

      // sf: bind mousekeys to virtual keyboard keys
      // ugh
      
      if(X_event.xbutton.state & Button1Mask ||
	 X_event.xbutton.button == Button1)
	{
	  event.type = ev_keydown;
	  event.data1 = KEYD_MOUSE1;
	  D_PostEvent(&event);
	}
      if(X_event.xbutton.state & Button2Mask ||
	 X_event.xbutton.button == Button2)
	{
	  event.type = ev_keydown;
	  event.data1 = KEYD_MOUSE2;
	  D_PostEvent(&event);
	}
      if(X_event.xbutton.state & Button3Mask ||
	 X_event.xbutton.button == Button3)
	{
	  event.type = ev_keydown;
	  event.data1 = KEYD_MOUSE3;
	  D_PostEvent(&event);
	}

      // fprintf(stderr, "b");
      break;
      
    case ButtonRelease:
      event.type = ev_mouse;
      event.data1 =
	(X_event.xbutton.state & Button1Mask ? 1 : 0)
	| (X_event.xbutton.state & Button2Mask ? 2 : 0)
	| (X_event.xbutton.state & Button3Mask ? 4 : 0);
      // suggest parentheses around arithmetic in operand of |
      event.data1 =
	event.data1
	^ (X_event.xbutton.button == Button1 ? 1 : 0)
	^ (X_event.xbutton.button == Button2 ? 2 : 0)
	^ (X_event.xbutton.button == Button3 ? 4 : 0);
      event.data2 = event.data3 = 0;
      D_PostEvent(&event);
      // fprintf(stderr, "bu");

      // sf: virtual mousekeys     
      if((X_event.xbutton.state & Button1Mask) ^
	 (X_event.xbutton.button == Button1))
	{
	  event.type = ev_keyup;
	  event.data1 = KEYD_MOUSE1;
	  D_PostEvent(&event);
	}
      if((X_event.xbutton.state & Button2Mask) ^
	 (X_event.xbutton.button == Button2))
	{
	  event.type = ev_keyup;
	  event.data1 = KEYD_MOUSE2;
	  D_PostEvent(&event);
	}
      if((X_event.xbutton.state & Button3Mask) ^
	 (X_event.xbutton.button == Button3))
	{
	  event.type = ev_keyup;
	  event.data1 = KEYD_MOUSE3;
	  D_PostEvent(&event);
	}
      break;
      
    case MotionNotify:
      // If the event is from warping the pointer back to middle
      // of the screen then ignore it.
      if ((X_event.xmotion.x == X_width/2) &&
	  (X_event.xmotion.y == X_height/2)) 
	{
	  lastmousex = X_event.xmotion.x;
	  lastmousey = X_event.xmotion.y;
	  break;
	} 
      else
	{
	  event.data2 = (X_event.xmotion.x - lastmousex) << 2;
	  event.data3 = (lastmousey - X_event.xmotion.y) << 2;
	  lastmousex = X_event.xmotion.x;
	  lastmousey = X_event.xmotion.y;
	}
      event.type = ev_mouse;
      event.data1 =
	(X_event.xmotion.state & Button1Mask ? 1 : 0)
	| (X_event.xmotion.state & Button2Mask ? 2 : 0)
	| (X_event.xmotion.state & Button3Mask ? 4 : 0);
      D_PostEvent(&event);
      // fprintf(stderr, "m");
      // Warp the pointer back to the middle of the window
      //  or we cannot move any further if it's at a border.
      if (grabnow)
	{
	  if ((X_event.xmotion.x < 1) || (X_event.xmotion.y < 1)
	      || (X_event.xmotion.x > X_width-2)
	      || (X_event.xmotion.y > X_height-2))
	    {
	      XWarpPointer(X_display,
			   None,
			   X_mainWindow,
			   0, 0,
			   0, 0,
			   X_width/2, X_height/2);
	    }
	}
      break;

      // sf: quit detection from lxdoom
      
    case ClientMessage:
      if (X_event.xclient.data.l[0] == X_deletewin)
	  C_RunTextCmd("mn_quit");
    break;

    case Expose:
    case ConfigureNotify:
      break;
      
    default:
#ifndef NOSHM
      if (doShm && X_event.type == X_shmeventtype)
	shmFinished = true;
#endif
      break;
      
    }
  
}

static Cursor createnullcursor(Display* display, Window root)
{
  Pixmap cursormask;
  XGCValues xgc;
  GC gc;
  XColor dummycolour;
  static Cursor cursor;
  static boolean cursor_made = false;

  // sf: make cursor if we havent already

  if(!cursor_made)
    {
      cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
      xgc.function = GXclear;
      gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
      XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
      dummycolour.pixel = 0;
      dummycolour.red = 0;
      dummycolour.flags = 04;
      cursor = XCreatePixmapCursor(display, cursormask, cursormask,
				   &dummycolour,&dummycolour, 0,0);
      XFreePixmap(display,cursormask);
      XFreeGC(display,gc);

      cursor_made = true;
    }

  return cursor;
}

//
// XWin_StartTic
//
void XWin_StartTic(void)
{
  if (!X_display)
    return;
  
  if (Mousegrabbed && !grabnow)
    {
      XUndefineCursor(X_display, X_mainWindow);
      XUngrabPointer(X_display, CurrentTime);
      Mousegrabbed = false;
    } 
  else if (!Mousegrabbed && grabnow)
    {
      XGrabPointer(X_display, X_mainWindow, True,
		   ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
		   GrabModeAsync, GrabModeAsync,
		   X_mainWindow, None, CurrentTime);
      XDefineCursor(X_display, X_mainWindow,
		    createnullcursor( X_display, X_mainWindow ) );
      Mousegrabbed = true;
    }
  
  while (XPending(X_display))
    XWin_GetEvent();
}

//
// Disk icon code: not implemented in Linux
//

void XWin_BeginRead()
{
}

void XWin_EndRead()
{
}


//
// XWin_UpdateNoBlit
//
void XWin_UpdateNoBlit(void)
{
  /* empty */
}

//
// XWin_FinishUpdate
//
void XWin_FinishUpdate(void)
{  
  // sf: fps dots now non-sys specific
  
  // Special optimization for 16bpp and screen size * 2, because this
  // probably is the most used one. This code does the scaling
  // and colormap transformation in one single loop instead of two,
  // which halves the time needed for the transformations.
  if ((multiply == 2) && (x_bpp == 2))
    {
      unsigned int *olineptrs[2];
      unsigned char *ilineptr;
      int x, y, i;
      unsigned short pixel;
      unsigned int twopixel;
      int step = X_width/2;
      
      ilineptr = (unsigned char *) screens[0];
      for (i=0 ; i<2 ; i++)
	olineptrs[i] = (unsigned int *) &(image->data[i*X_width*2]);
      
      y = baseheight-1;
      do
	{
	  x = basewidth-1;
	  do
	    {
	      pixel = x_colormap2[*ilineptr++];
	      twopixel = (pixel << 16) | pixel;
	      *olineptrs[0]++ = twopixel;
	      *olineptrs[1]++ = twopixel;
	    } while (x--);
	  olineptrs[0] += step;
	  olineptrs[1] += step;
	} while (y--);
      goto blit_it;	// indenting the whole stuff with one more
      // elseif won't make it better
    }
  
  // Special optimization for 16bpp and screen size * 3, because I
  // just was working on it anyway. This code does the scaling
  // and colormap transformation in one single loop instead of two,
  // which halves the time needed for the transformations.
  if ((multiply == 3) && (x_bpp == 2))
    {
      unsigned short *olineptrs[3];
      unsigned char *ilineptr;
      int x, y, i;
      unsigned short pixel;
      int step = 2*X_width;
      
      ilineptr = (unsigned char *) screens[0];
      for (i=0 ; i<3 ; i++)
	olineptrs[i] = (unsigned short *) &(image->data[i*X_width*2]);

      y = baseheight-1;
      do
	{
	  x = basewidth-1;
	  do
	    {
	      pixel = x_colormap2[*ilineptr++];
	      *olineptrs[0]++ = pixel;
	      *olineptrs[0]++ = pixel;
	      *olineptrs[0]++ = pixel;
	      *olineptrs[1]++ = pixel;
	      *olineptrs[1]++ = pixel;
	      *olineptrs[1]++ = pixel;
	      *olineptrs[2]++ = pixel;
	      *olineptrs[2]++ = pixel;
	      *olineptrs[2]++ = pixel;
	    } while (x--);
	  olineptrs[0] += step;
	  olineptrs[1] += step;
	  olineptrs[2] += step;
	} while (y--);
      goto blit_it;	// indenting the whole stuff with one more
      // elseif won't make it better
    }
  
  // From here on the old code, first scale screen, then in a second step
  // do the colormap transformation. This works for all combinations.
  
  // scales the screen size before blitting it
  if (multiply == 2)
    {
      unsigned int *olineptrs[2];
      unsigned int *ilineptr;
      int x, y, i;
      unsigned int twoopixels;
      unsigned int twomoreopixels;
      unsigned int fouripixels;
      unsigned int step = X_width/4;
      
      ilineptr = (unsigned int *) (screens[0]);
      for (i=0 ; i<2 ; i++)
	olineptrs[i] = (unsigned int *) &image->data[i*X_width];
      
      y = baseheight;
      while (y--)
	{
	  x = basewidth;
	  do
	    {
	      fouripixels = *ilineptr++;
	      twoopixels =	(fouripixels & 0xff000000)
		|	((fouripixels>>8) & 0xffff00)
		|	((fouripixels>>16) & 0xff);
	      twomoreopixels =	((fouripixels<<16) & 0xff000000)
		|	((fouripixels<<8) & 0xffff00)
		|	(fouripixels & 0xff);
#ifdef BIGEND
	      *olineptrs[0]++ = twoopixels;
	      *olineptrs[1]++ = twoopixels;
	      *olineptrs[0]++ = twomoreopixels;
	      *olineptrs[1]++ = twomoreopixels;
#else
	      *olineptrs[0]++ = twomoreopixels;
	      *olineptrs[1]++ = twomoreopixels;
	      *olineptrs[0]++ = twoopixels;
	      *olineptrs[1]++ = twoopixels;
#endif
	    } while (x-=4);
	  olineptrs[0] += step;
	  olineptrs[1] += step;
	}
      
    }
  else if (multiply == 3)
    {
      unsigned int *olineptrs[3];
      unsigned int *ilineptr;
      int x, y, i;
      unsigned int fouropixels[3];
      unsigned int fouripixels;
      unsigned int step = 2*X_width/4;
      
      ilineptr = (unsigned int *) (screens[0]);
      for (i=0 ; i<3 ; i++)
	olineptrs[i] = (unsigned int *) &image->data[i*X_width];
      
      y = baseheight;
      while (y--)
	{
	  x = basewidth;
	  do
	    {
	      fouripixels = *ilineptr++;
	      fouropixels[0] = (fouripixels & 0xff000000)
		|	((fouripixels>>8) & 0xff0000)
		|	((fouripixels>>16) & 0xffff);
	      fouropixels[1] = ((fouripixels<<8) & 0xff000000)
		|	(fouripixels & 0xffff00)
		|	((fouripixels>>8) & 0xff);
	      fouropixels[2] = ((fouripixels<<16) & 0xffff0000)
		|	((fouripixels<<8) & 0xff00)
		|	(fouripixels & 0xff);
#ifdef BIGEND
	      *olineptrs[0]++ = fouropixels[0];
	      *olineptrs[1]++ = fouropixels[0];
	      *olineptrs[2]++ = fouropixels[0];
	      *olineptrs[0]++ = fouropixels[1];
	      *olineptrs[1]++ = fouropixels[1];
	      *olineptrs[2]++ = fouropixels[1];
	      *olineptrs[0]++ = fouropixels[2];
	      *olineptrs[1]++ = fouropixels[2];
	      *olineptrs[2]++ = fouropixels[2];
#else
	      *olineptrs[0]++ = fouropixels[2];
	      *olineptrs[1]++ = fouropixels[2];
	      *olineptrs[2]++ = fouropixels[2];
	      *olineptrs[0]++ = fouropixels[1];
	      *olineptrs[1]++ = fouropixels[1];
	      *olineptrs[2]++ = fouropixels[1];
	      *olineptrs[0]++ = fouropixels[0];
	      *olineptrs[1]++ = fouropixels[0];
	      *olineptrs[2]++ = fouropixels[0];
#endif
	    } while (x-=4);
	  olineptrs[0] += step;
	  olineptrs[1] += step;
	  olineptrs[2] += step;
	}
    }
  else if (multiply == 4)
    {
      // Broken. Gotta fix this some day.
      static void Expand4(unsigned *, double *);
      Expand4 ((unsigned *)(screens[0]), (double *) (image->data));
    }
  
  // colormap transformation dependend on X server color depth
  if (x_bpp == 2)
    {
      int x,y;
      int xstart = basewidth*multiply-1;
      unsigned char* ilineptr;
      unsigned short* olineptr;
      y = baseheight*multiply;      
      while (y--) 
	{
	  olineptr =  (unsigned short *) &(image->data[y*X_width*x_bpp]);
	  if (multiply==1)
	    ilineptr = (unsigned char*) (screens[0]+y*X_width);
	  else
	    ilineptr =  (unsigned char*) &(image->data[y*X_width]);
	  x = xstart;
	  do 
	    {
	      olineptr[x] = x_colormap2[ilineptr[x]];
	    } while (x--);
	}
    }
  else if (x_bpp == 3)
    {
      int x,y;
      int xstart = basewidth*multiply-1;
      unsigned char* ilineptr;
      unsigned char* olineptr;
      y = baseheight*multiply;
      while (y--)
	{
	  olineptr =  (unsigned char *) &image->data[y*X_width*x_bpp];
	  if (multiply==1)
	    ilineptr = (unsigned char*) (screens[0]+y*X_width);
	  else
	    ilineptr =  (unsigned char*) &image->data[y*X_width];
	  x = xstart;
	  do
	    {
	      memcpy(olineptr+3*x,x_colormap3+3*ilineptr[x],3);
	    } while (x--);
	}
    }
  else if (x_bpp == 4)
    {
      int x,y;
      int xstart = basewidth*multiply-1;
      unsigned char* ilineptr;
      unsigned int* olineptr;
      y = baseheight*multiply;      
      while (y--) 
	{
	  olineptr =  (unsigned int *) &(image->data[y*X_width*x_bpp]);
	  if (multiply==1)
	    ilineptr = (unsigned char*) (screens[0]+y*X_width);
	  else
	    ilineptr =  (unsigned char*) &(image->data[y*X_width]);
	  x = xstart;
	  do 
	    {
	      olineptr[x] = x_colormap4[ilineptr[x]];
	    } while (x--);
	}
    }
  
 blit_it:
#ifndef NOSHM
  if (doShm)
    {
      if (!XShmPutImage(X_display,
		       	X_mainWindow,
		       	X_gc,
		       	image,
		       	0, 0,
		       	0, 0,
		       	X_width, X_height,
		       	True ))
	I_Error("XShmPutImage() failed\n");
      
      // wait for it to finish and processes all input events
      shmFinished = false;
      do
	{
	  XWin_GetEvent();
	} while (!shmFinished);
      
    }
  else
#endif /* #ifndef NOSHM */
    {      
      // draw the image
      XPutImage(X_display,
	       	X_mainWindow,
	       	X_gc,
	       	image,
	       	0, 0,
		0, 0,
		X_width, X_height );
    }
}


//
// XWin_ReadScreen
//
void XWin_ReadScreen(byte* scr)
{
  memcpy (scr, screens[0], basewidth*baseheight);
}


//
// Palette stuff.
//
static XColor colors[256];

static void UploadNewPalette(Colormap cmap, byte *palette)
{
  register int	i;
  register int	c;
  static boolean firstcall = true;
  
#ifdef __cplusplus
  if (X_visualinfo.c_class == PseudoColor && X_visualinfo.depth == 8)
#else
    if (X_visualinfo.class == PseudoColor && X_visualinfo.depth == 8)
#endif
      {
	// initialize the colormap
	if (firstcall)
	  {
	    firstcall = false;
	    for (i=0 ; i<256 ; i++)
	      {
		colors[i].pixel = i;
		colors[i].flags = DoRed|DoGreen|DoBlue;
	      }
	  }
	
	// set the X colormap entries
	for (i=0 ; i<256 ; i++)
	  {
	    c = gammatable[usegamma][*palette++];
	    colors[i].red = (c<<8) + c;
	    c = gammatable[usegamma][*palette++];
	    colors[i].green = (c<<8) + c;
	    c = gammatable[usegamma][*palette++];
	    colors[i].blue = (c<<8) + c;
	  }
	
	// store the colors to the current colormap
	XStoreColors(X_display, cmap, colors, 256);	
      }
}

static void EmulateNewPalette(byte *palette)
{
  register int	i;
  
  for (i=0 ; i<256 ; i++) 
    {
      if (x_bpp==2) 
	{
	  x_colormap2[i] =
	    ((gammatable[usegamma][*palette]>>x_red_mask)<<x_red_offset) |
	    ((gammatable[usegamma][palette[1]]>>x_green_mask)<<x_green_offset) |
	    ((gammatable[usegamma][palette[2]]>>x_blue_mask)<<x_blue_offset);
	  palette+=3;  
	} 
      else if (x_bpp==3)
	{
	  x_colormap3[3*i+x_red_offset] = gammatable[usegamma][*palette++];
	  x_colormap3[3*i+x_green_offset] = gammatable[usegamma][*palette++];
	  x_colormap3[3*i+x_blue_offset] = gammatable[usegamma][*palette++];  
	} 
      else if (x_bpp==4) 
	{
	  x_colormap4[i] = 0;
	  ((unsigned char*)(x_colormap4+i))[x_red_offset] =
	    gammatable[usegamma][*palette++];
	  ((unsigned char*)(x_colormap4+i))[x_green_offset] =
	    gammatable[usegamma][*palette++];
	  ((unsigned char*)(x_colormap4+i))[x_blue_offset] =
	    gammatable[usegamma][*palette++];
	}  
    }
}

//
// XWin_SetPalette
//
void XWin_SetPalette(byte* palette)
{
  if (x_pseudo)
    UploadNewPalette(X_cmap, palette);
  else
    EmulateNewPalette(palette);
}


#ifndef NOSHM 

//
// This function is probably redundant,
//  if XShmDetach works properly.
// ddt never detached the XShm memory,
//  thus there might have been stale
//  handles accumulating.
//
static void grabsharedmemory(int size)
{
  int			key = ('d'<<24) | ('o'<<16) | ('o'<<8) | 'm';
  struct shmid_ds	shminfo;
  int			minsize = 320*200;
  int			id;
  int			rc;
  // UNUSED int done=0;
  int			pollution=5;
  
  // try to use what was here before
  do
    {
      id = shmget((key_t) key, minsize, 0777); // just get the id
      if (id != -1)
	{
	  rc=shmctl(id, IPC_STAT, &shminfo); // get stats on it
	  if (!rc) 
	    {
	      if (shminfo.shm_nattch)
		{
		  fprintf(stderr, "User %d appears to be running "
			  "DOOM.  Is that wise?\n", shminfo.shm_cpid);
		  key++;
		}
	      else
		{
		  if (getuid() == shminfo.shm_perm.cuid)
		    {
		      rc = shmctl(id, IPC_RMID, 0);
		      if (!rc)
			fprintf(stderr,
				"Was able to kill my old shared memory\n");
		      else
			I_Error("Was NOT able to kill my old shared memory");
		      
		      id = shmget((key_t)key, size, IPC_CREAT|0777);
		      if (id==-1)
			I_Error("Could not get shared memory");

		      rc=shmctl(id, IPC_STAT, &shminfo);
		      
		      break;
		    }
		  if (size >= shminfo.shm_segsz)
		    {
		      fprintf(stderr,
			      "will use %d's stale shared memory\n",
			      shminfo.shm_cpid);
		      break;
		    }
		  else
		    {
		      fprintf(stderr,
			      "warning: can't use stale "
			      "shared memory belonging to id %d, "
			      "key=0x%x\n",
			      shminfo.shm_cpid, key);
		      key++;
		    }
		}
	    }
	  else
	    {
	      I_Error("could not get stats on key=%d", key);
	    }
	}
      else
	{
	  id = shmget((key_t)key, size, IPC_CREAT|0777);
	  if (id==-1)
	    {
	      extern int errno;
	      fprintf(stderr, "errno=%d\n", errno);
	      I_Error("Could not get any shared memory");
	    }
	  break;
	}
    } while (--pollution);
  
  if (!pollution)
    {
      I_Error("Sorry, system too polluted with stale "
	      "shared memory segments.\n");
    }	
  
  X_shminfo.shmid = id;
  
  // attach to the shared memory segment
  image->data = X_shminfo.shmaddr = shmat(id, 0, 0);
  
  fprintf(stderr, "shared memory id=%d, addr=0x%x\n", id,
	  (int) (image->data));
}

#endif /* #ifndef NOSHM */

//
// XWin_CloseWindow
//
// Close the main window
//

static void XWin_CloseWindow()
{
  if(!window_open)
    return;

  if(X_display && X_mainWindow)
    XDestroyWindow(X_display, X_mainWindow);

  window_open = false;
  
#ifndef NOSHM
  // Release shared memory.

  // Detach from X server
  if (!XShmDetach(X_display, &X_shminfo))
    I_Error("XShmDetach() failed in I_ShutdownGraphics()");

  shmdt(X_shminfo.shmaddr);
#endif
}

//
// XWin_OpenWindow
//
// Open the main window
//

static void XWin_OpenWindow()
{
  int			oktodraw;
  XGCValues		xgcvalues;
  int			valuemask;
  Window		dummy;
  int			dont_care;
  int                   x=0, y=0;
  unsigned long	attribmask;
  XSetWindowAttributes attribs;

  // if window open, close it first

  if(window_open)
    XWin_CloseWindow();

  // set up width, height

  basewidth = SCREENWIDTH << hires;
  baseheight = SCREENHEIGHT << hires;

  X_width = basewidth * multiply;
  X_height = baseheight * multiply;


#ifndef NOSHM

  // set up shm

  if (doShm)
    {
      X_shmeventtype = XShmGetEventBase(X_display) + ShmCompletion;
      
      // create the image
      image = XShmCreateImage(X_display,
			      X_visual,
			      8*x_depth,
			      ZPixmap,
			      0,
			      &X_shminfo,
			      X_width,
			      X_height );
      
      grabsharedmemory(image->bytes_per_line * image->height);
      
      
      // UNUSED
      // create the shared memory segment
      // X_shminfo.shmid = shmget (IPC_PRIVATE,
      // image->bytes_per_line * image->height, IPC_CREAT | 0777);
      // if (X_shminfo.shmid < 0)
      // {
      // perror("");
      // I_Error("shmget() failed in InitGraphics()");
      // }
      // fprintf(stderr, "shared memory id=%d\n", X_shminfo.shmid);
      // attach to the shared memory segment
      // image->data = X_shminfo.shmaddr = shmat(X_shminfo.shmid, 0, 0);
      
      
      if (!image->data)
	{
	  perror("");
	  I_Error("shmat() failed in InitGraphics()");
	}
      
      // get the X server to attach to it
      if (!XShmAttach(X_display, &X_shminfo))
	I_Error("XShmAttach() failed in InitGraphics()");
      
    }
  else
#endif /* #ifndef NOSHM */
    {
      image = XCreateImage(X_display,
			   X_visual,
			   8*x_depth,
			   ZPixmap,
			   0,
			   (char*)malloc(X_width * X_height * x_depth),
			   X_width, X_height,
			   8*x_depth,
			   X_width*x_bpp );
      
    }
  
  if (multiply == 1 && x_depth == 1)
    screens[0] = (unsigned char *) (image->data);
  else
    screens[0] = (unsigned char *) malloc (basewidth * baseheight);

  // setup attributes for main window
  attribmask = CWEventMask | CWColormap | CWBorderPixel;
  attribs.event_mask =
    KeyPressMask
    | KeyReleaseMask
    | PointerMotionMask | ButtonPressMask | ButtonReleaseMask
    | ExposureMask;
  
  attribs.colormap = X_cmap;
  attribs.border_pixel = 0;

  // create the main window
  X_mainWindow = XCreateWindow(X_display,
			       RootWindow(X_display, X_screen),
			       x, y,
			       X_width, X_height,
			       0, // borderwidth
			       8*x_depth, // depth
			       InputOutput,
			       X_visual,
			       attribmask,
			       &attribs );

  // sf: allow close by clicking close button in WM
  // from lxdoom
  XSetWMProtocols(X_display, X_mainWindow, &X_deletewin, 1);
  
  // set window name
  XStoreName(X_display, X_mainWindow, "SMMU");
  
  // create the GC
  valuemask = GCGraphicsExposures;
  xgcvalues.graphics_exposures = False;
  X_gc = XCreateGC(X_display,
		   X_mainWindow,
		   valuemask,
		   &xgcvalues );
  
  // map the window
  XMapWindow(X_display, X_mainWindow);
  
  // wait until it is OK to draw
  oktodraw = 0;
  while (!oktodraw)
    {
      XNextEvent(X_display, &X_event);
      if (X_event.type == Expose
	  && !X_event.xexpose.count)
	{
	  oktodraw = 1;
	}
    }

  Mousegrabbed = false;
    
  // set the focus to the window, some window managers don't and
  // we're in deep shit if option -grabmouse was used
  XSetInputFocus(X_display, X_mainWindow, RevertToPointerRoot, CurrentTime);
  
  // get the pointer coordinates so that the first pointer move won't
  // be completely wrong
  XQueryPointer(X_display, X_mainWindow, &dummy, &dummy,
		&dont_care, &dont_care,
		&lastmousex, &lastmousey,
		&dont_care);

  window_open = true;          // window is open
}

//---------------------------------------------------------------------------
//
// XWin_InitGraphics
//
// Connect to X server and initialise xwin driver
// returns true if successfully initted
//

boolean XWin_InitGraphics(void)
{
  char*		displayname = NULL;
  char*		d;
  int			n;
  int			pnum;
  int			x=0;
  int			y=0;
  
  // warning: char format, different type arg
  char		xsign=' ';
  char		ysign=' ';
  
  static boolean firsttime=true;
    
  if (!firsttime)
    return true;
  firsttime = false;

  // check for command-line display name
  if ( (pnum=M_CheckParm("-disp")) ) // suggest parentheses around assignment
    displayname = myargv[pnum+1];
  else
    displayname = NULL;

  // check if the user wants to grab the mouse (quite unnice)
  if(M_CheckParm("-grabmouse"))
    grabMouse = true;

  // check for command-line geometry
  if ( (pnum=M_CheckParm("-geom")) ) // suggest parentheses around assignment
    {
      // warning: char format, different type arg 3,5
      n = sscanf(myargv[pnum+1], "%c%d%c%d", &xsign, &x, &ysign, &y);
	
      if (n==2)
	x = y = 0;
      else if (n==6)
	{
	  if (xsign == '-')
	    x = -x;
	  if (ysign == '-')
	    y = -y;
	}
      else
	I_Error("bad -geom parameter");
    }
  
  // open the display
  X_display = XOpenDisplay(displayname);
  if (!X_display)
    {
      if (displayname)
        printf("Could not open display [%s]\n", displayname);
      else
	printf("Could not open display (DISPLAY=[%s])\n", getenv("DISPLAY"));

      return false;         // could not init
    }

  // exit by clicking close button in wm

  X_deletewin = XInternAtom(X_display, "WM_DELETE_WINDOW", False);

  // use the default visual 
  X_screen = DefaultScreen(X_display);
  if (XMatchVisualInfo(X_display, X_screen, 8, PseudoColor, &X_visualinfo))
    { x_depth = 1; x_pseudo = 1; }
  else if
    (XMatchVisualInfo(X_display, X_screen, 16, TrueColor, &X_visualinfo))
    { x_depth = 2; x_pseudo = 0; }
  else if
    (XMatchVisualInfo(X_display, X_screen, 24, TrueColor, &X_visualinfo))
    { x_depth = 3; x_pseudo = 0; }
  else
    I_Error("no supported visual found");
  X_visual = X_visualinfo.visual;
  x_red_mask = X_visual->red_mask;
  x_green_mask = X_visual->green_mask;
  x_blue_mask = X_visual->blue_mask;
  
  if (x_depth==3) 
    {
      switch (x_red_mask)
	{
#ifdef BIGEND
	case 0x000000ff: x_red_offset = 3; break;
	case 0x0000ff00: x_red_offset = 2; break;
	case 0x00ff0000: x_red_offset = 1; break;
	case 0xff000000: x_red_offset = 0; break;
#else
	case 0x000000ff: x_red_offset = 0; break;
	case 0x0000ff00: x_red_offset = 1; break;
	case 0x00ff0000: x_red_offset = 2; break;
	case 0xff000000: x_red_offset = 3; break;
#endif
	}
      switch (x_green_mask) 
	{
#ifdef BIGEND
	case 0x000000ff: x_green_offset = 3; break;
	case 0x0000ff00: x_green_offset = 2; break;
	case 0x00ff0000: x_green_offset = 1; break;
	case 0xff000000: x_green_offset = 0; break;
#else
	case 0x000000ff: x_green_offset = 0; break;
	case 0x0000ff00: x_green_offset = 1; break;
	case 0x00ff0000: x_green_offset = 2; break;
	case 0xff000000: x_green_offset = 3; break;
#endif
	}
      switch (x_blue_mask) 
	{
#ifdef BIGEND
	case 0x000000ff: x_blue_offset = 3; break;
	case 0x0000ff00: x_blue_offset = 2; break;
	case 0x00ff0000: x_blue_offset = 1; break;
	case 0xff000000: x_blue_offset = 0; break;
#else
	case 0x000000ff: x_blue_offset = 0; break;
	case 0x0000ff00: x_blue_offset = 1; break;
	case 0x00ff0000: x_blue_offset = 2; break;
	case 0xff000000: x_blue_offset = 3; break;
#endif
	}
    }
  if (x_depth==2) 
    {
      // for 16bpp, x_*_offset specifies the number of bits to shift
      unsigned long mask;
      
      mask = x_red_mask;
      x_red_offset = 0;
      while (!(mask&1)) 
	{
	  x_red_offset++;
	  mask >>= 1;
	}
      x_red_mask = 8;
      while (mask&1) 
	{
	  x_red_mask--;
	  mask >>= 1;
	}

      mask = x_green_mask;
      x_green_offset = 0;
      while (!(mask&1))
	{
	  x_green_offset++;
	  mask >>= 1;
	}
      x_green_mask = 8;
      while (mask&1) 
	{
	  x_green_mask--;
	  mask >>= 1;
	}
      
      mask = x_blue_mask;
      x_blue_offset = 0;
      while (!(mask&1)) 
	{
	  x_blue_offset++;
	  mask >>= 1;
	}
      x_blue_mask = 8;
      while (mask&1) 
	{
	  x_blue_mask--;
	  mask >>= 1;
	}
    }

  {
    int count;
    XPixmapFormatValues* X_pixmapformats =
      XListPixmapFormats(X_display,&count);
    if (X_pixmapformats) 
      {
	int i;
	x_bpp=0;
	for (i=0;i<count;i++) 
	  {
	    if (X_pixmapformats[i].depth == x_depth*8) 
	      {
		x_bpp = X_pixmapformats[i].bits_per_pixel/8; break;
	      }
	  }
	if (x_bpp==0)
	  I_Error("Could not determine bits_per_pixel");
	XFree(X_pixmapformats);
      } 
    else
      I_Error("Could not get list of pixmap formats");
  }

#ifndef NOSHM
  
  // check for the MITSHM extension
  doShm = 
    !M_CheckParm("-noshm") &&
    XShmQueryExtension(X_display);
  
  // even if it's available, make sure it's a local connection
  if (doShm)
    {
      if (!displayname) displayname = (char *) getenv("DISPLAY");
      if (displayname)
	{
	  d = displayname;
	  while (*d && (*d != ':')) d++;
	  if (*d) *d = 0;
	  if (!(!strcasecmp(displayname, "unix") || !*displayname))
	    doShm = false;
	}
    }
  
  if (doShm)
    fprintf(stderr, "Using MITSHM extension\n");

#endif /* #ifndef NOSHM */
  
  // create the colormap
  if (x_pseudo)
    X_cmap = XCreateColormap(X_display, RootWindow(X_display, X_screen),
			     X_visual, AllocAll);
  else if (x_bpp==2)
    x_colormap2 = malloc(2*256);
  else if (x_bpp==3)
    x_colormap3 = malloc(3*256);
  else if (x_bpp==4)
    x_colormap4 = malloc(4*256);

  // add console cmds

  XWin_AddCommands();

  return true;                     // initted ok
}


//  static unsigned	exptable[256];

//  static void InitExpand(void)
//  {
//    int		i;
  
//    for (i=0 ; i<256 ; i++)
//      exptable[i] = i | (i<<8) | (i<<16) | (i<<24);
//  }

static double exptable2[256*256];

static void InitExpand2(void)
{
  int		i;
  int		j;
  // UNUSED unsigned	iexp, jexp;
  double*	exp;
  union
  {
    double 		d;
    unsigned	u[2];
  } pixel;
  
  printf ("building exptable2...\n");
  exp = exptable2;
  for (i=0 ; i<256 ; i++)
    {
      pixel.u[0] = i | (i<<8) | (i<<16) | (i<<24);
      for (j=0 ; j<256 ; j++)
	{
	  pixel.u[1] = j | (j<<8) | (j<<16) | (j<<24);
	  *exp++ = pixel.d;
	}
    }
  printf ("done.\n");
}

static int inited;

static void Expand4(unsigned* lineptr, double* xline)
{
  double	dpixel;
  unsigned	x;
  unsigned 	y;
  unsigned	fourpixels;
  unsigned	step;
  double*	exp;
  
  exp = exptable2;
  if (!inited)
    {
      inited = 1;
      InitExpand2 ();
    }
  
  step = 3*basewidth/2;
  
  y = baseheight-1;
  do
    {
      x = basewidth;
      
      do
	{
	  fourpixels = lineptr[0];
	  
	  dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	  xline[0] = dpixel;
	  xline[160] = dpixel;
	  xline[320] = dpixel;
	  xline[480] = dpixel;
	  
	  dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	  xline[1] = dpixel;
	  xline[161] = dpixel;
	  xline[321] = dpixel;
	  xline[481] = dpixel;
	  
	  fourpixels = lineptr[1];
	  
	  dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	  xline[2] = dpixel;
	  xline[162] = dpixel;
	  xline[322] = dpixel;
	  xline[482] = dpixel;
	  
	  dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	  xline[3] = dpixel;
	  xline[163] = dpixel;
	  xline[323] = dpixel;
	  xline[483] = dpixel;
	  
	  fourpixels = lineptr[2];
	  
	  dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	  xline[4] = dpixel;
	  xline[164] = dpixel;
	  xline[324] = dpixel;
	  xline[484] = dpixel;
	  
	  dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	  xline[5] = dpixel;
	  xline[165] = dpixel;
	  xline[325] = dpixel;
	  xline[485] = dpixel;
	  
	  fourpixels = lineptr[3];
	  
	  dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	  xline[6] = dpixel;
	  xline[166] = dpixel;
	  xline[326] = dpixel;
	  xline[486] = dpixel;
	  
	  dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	  xline[7] = dpixel;
	  xline[167] = dpixel;
	  xline[327] = dpixel;
	  xline[487] = dpixel;
	  
	  lineptr+=4;
	  xline+=8;
	} while (x-=16);
      xline += step;
    } while (y--);
}

char *xwin_modenames[] =
{
  "320x200",
  "320x200 2x stretch",
  "320x200 3x stretch",
  "320x200 4x stretch",
  "640x400",
  "640x400 2x stretch",
  NULL
};

boolean XWin_SetMode(int i)
{
  switch(i)
    {
    default:
    case 0: hires = 0; multiply = 1; break;
    case 1: hires = 0; multiply = 2; break;
    case 2: hires = 0; multiply = 3; break;
    case 3: hires = 0; multiply = 4; break;
    case 4: hires = 1; multiply = 1; break;
    case 5: hires = 1; multiply = 2; break;
    }

  // open the new window
  XWin_OpenWindow();

  return true;
}

// Unset mode ie. close window

void XWin_UnsetMode()
{
  XWin_CloseWindow();
}

viddriver_t xwin_driver =
  {
    "X Window",
    "-xwin",

    XWin_InitGraphics,
    XWin_ShutdownGraphics,

    XWin_SetMode,
    XWin_UnsetMode,

    XWin_FinishUpdate,
    XWin_SetPalette,

    XWin_StartTic,
    xwin_modenames
  };

#endif /* #ifdef XWIN */

//--------------------------------------------------------------------------
//
// $Log:$
//
//--------------------------------------------------------------------------
