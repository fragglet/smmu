// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Windows interface courtesy of prboom
//
//-----------------------------------------------------------------------------


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wingdi.h>
//#include <stdlib.h>

#define NOASM
#define DIRECTX

// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef _MSC_VER
#define DIRECTX
#endif
#ifdef DIRECTX
//#define __BYTEBOOL__
//#define false 0
//#define true !false
#include <ddraw.h>
#endif

#include "../doomtype.h"
#include "../doomdef.h"
#include "../m_argv.h"
#include "../d_event.h"
#include "../d_main.h"
#include "../v_video.h"
#include "../i_system.h"
#include "../v_mode.h"
#include "../v_video.h"
#include "../z_zone.h"

// i_main.c

extern HINSTANCE main_hInstance;

extern int usemouse;
static char title[128] = "SMMU";
static char szTitle[128];

static WNDCLASS wndclass;  //sf: globalled

HWND ghWnd;
static char szAppName[] = "PrBoomWinClass";
static char szTitlePrefix[] = "";
static HINSTANCE win_hInstance;
static int frameX, frameY, capY;

static int multiply;   //sf: globaled
// proff 06/30/98: Changed form constant value to defined value
// proff 08/17/98: Changed for high-res
//int MainWinWidth=SCREENWIDTH, MainWinHeight=SCREENHEIGHT;
static int MainWinWidth;
static int MainWinHeight;

static BITMAPINFO *View_bmi;
static BYTE *ViewMem;
static BYTE *ScaledVMem;
// proff 06/30/98: Changed form constant value to defined value
// proff 08/17/98: Changed for high-res
//int ViewMemPitch=SCREENWIDTH;
static int ViewMemPitch;
static RECT ViewRect;
static HDC ViewDC = 0;
enum
  {
    Scale_None,
    Scale_Windows,
    Scale_Own
  } ViewScale;

static boolean fActive = false;
// proff: Removed fFullscreen
static int vidFullScreen = 0;

static boolean mouse_grabbed;

static boolean noMouse = false;
static int MouseButtons = 0;

static boolean noidle=false;

//boolean noDDraw = true;
#ifdef DIRECTX
LPDIRECTDRAW lpDD;
LPDIRECTDRAWSURFACE lpDDPSF;
LPDIRECTDRAWSURFACE lpDDBSF;
LPDIRECTDRAWPALETTE lpDDPal;
PALETTEENTRY ADDPal[256];
int BestWidth,BestHeight;
#endif

//===========================================================================
//
// Scancode2Doomcode (removed from SMMU previously)
//
//

static unsigned char key_ascii_table[128] =
{
/* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F             */
   0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,   9,       /* 0 */
   'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13,  0,   'a', 's',     /* 1 */
   'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39,  '`', 0,   92,  'z', 'x', 'c', 'v',     /* 2 */
   'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   3,   3,   3,   3,   8,       /* 3 */
   3,   3,   3,   3,   3,   0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,       /* 4 */
   0,   0,   0,   127, 0,   0,   92,  3,   3,   0,   0,   0,   0,   0,   0,   0,       /* 5 */
   13,  0,   '/', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   127,     /* 6 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '/', 0,   0,   0,   0,   0        /* 7 */
};

static int I_ScanCode2DoomCode (int a)
{
  // proff: a was sometimes out of range
  if (a>127)
    return 0;
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

static int I_DoomCode2ScanCode (int a)
{
  static int inverse[256], cache;
  for (;cache<256;cache++)
    inverse[I_ScanCode2DoomCode(cache)]=cache;
  return inverse[a];
}

// end of scancode2doomcode

static void (*FullscreenProc)(int fullscreen);

static void WinFullscreen(int fullscreen)
{
  vidFullScreen=0;
  doom_printf("Fullscreen-Mode not available");
}

static CALLBACK
#ifdef CYGWIN    /* shut up compiler */
int
#endif
WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
  event_t event;
  boolean AltDown;
  
  switch (iMsg)
    {
      // proff 07/29/98: Added WM_CLOSE
      case WM_CLOSE:
	return 1;
	break;
      case WM_MOVE:
      case WM_SIZE:
	mouse_grabbed = false;
	break;
      case WM_KILLFOCUS:
	// proff 08/18/98: This sets the priority-class
	if (!noidle)
	  SetPriorityClass (GetCurrentProcess(), IDLE_PRIORITY_CLASS);
	break;
      case WM_SETFOCUS:
	// proff 08/18/98: This sets the priority-class
	SetPriorityClass (GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	break;
      case WM_DESTROY:
	// proff 08/18/98: This sets the priority-class
	SetPriorityClass (GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	//      PostQuitMessage(0);
	break;
      case WM_ACTIVATE:
	fActive = (boolean)LOWORD(wParam);
	if (fActive)
	  {
	    doom_printf ("WM_ACTIVATE true");
	    event.type = ev_keyup;
	    event.data1 = KEYD_RCTRL;
	    event.data2 = 0;
	    event.data3 = 0;
	    D_PostEvent(&event);
	    event.data1 = KEYD_RALT;
	    D_PostEvent(&event);
	    MouseButtons=0;
	    event.type = ev_mouse;
	    event.data1 = MouseButtons;
	    event.data2 = 0;
	    event.data3 = 0;
	    D_PostEvent(&event);
	    mouse_grabbed = false;
	  }
	else
	  {
	    doom_printf ("WM_ACTIVATE false");
	  }
	break;
      case WM_SYSKEYDOWN:
      case WM_KEYDOWN:
	event.type = ev_keydown;
	event.data1 = I_ScanCode2DoomCode(((lParam >> 16) & 0x00ff));
	// proff 08/18/98: Now the pause-key works
	if (wParam==VK_PAUSE)
	  event.data1=KEYD_PAUSE;
	event.data2 = 0;
	event.data3 = 0;
	if ( event.data1 != 0 )
	  D_PostEvent(&event);
	AltDown = (GetAsyncKeyState(VK_MENU) < 0);
	if ((AltDown) & (wParam == VK_RETURN))
	  {
            vidFullScreen = 1-vidFullScreen;
            if (FullscreenProc)
	      FullscreenProc(vidFullScreen);
	  }
	break;
      case WM_SYSKEYUP:
      case WM_KEYUP:
	event.type = ev_keyup;
	event.data1 = I_ScanCode2DoomCode(((lParam >> 16) & 0x00ff));
	// proff 08/18/98: Now the pause-key works
	if (wParam==VK_PAUSE)
	  event.data1=KEYD_PAUSE;
	event.data2 = 0;
	event.data3 = 0;
	if ( event.data1 != 0 )
	  D_PostEvent(&event);
	break;
	
	// sf: modified (mouse buttons treated as keyboard buttons)
	
      case WM_LBUTTONDOWN:
	event.type = ev_keydown;
	event.data1 = KEYD_MOUSE1;
	if(!noMouse)
	D_PostEvent(&event);
	break;
	
      case WM_MBUTTONDOWN:
	event.type = ev_keydown;
	event.data1 = KEYD_MOUSE3;
	if(!noMouse)
	  D_PostEvent(&event);
	break;

      case WM_RBUTTONDOWN:
	event.type = ev_keydown;
	event.data1 = KEYD_MOUSE2;
	if(!noMouse)
	  D_PostEvent(&event);
	break;
	
      case WM_LBUTTONUP:
	event.type = ev_keyup;
	event.data1 = KEYD_MOUSE1;
	if(!noMouse)
	  D_PostEvent(&event);
	break;
	
      case WM_MBUTTONUP:
	event.type = ev_keyup;
	event.data1 = KEYD_MOUSE3;
	if(!noMouse)
	  D_PostEvent(&event);
	break;
	
      case WM_RBUTTONUP:
	event.type = ev_keyup;
	event.data1 = KEYD_MOUSE2;
	if(!noMouse)
	  D_PostEvent(&event);
	break;
	
      default:
        return(DefWindowProc(hwnd,iMsg,wParam,lParam));
    }
  // proff 08/18/98: Removed because I think it's useless
  //    return(DefWindowProc(hwnd,iMsg,wParam,lParam));

  return 0;
}

#ifdef CONSOLE

// Variables for the console
static HWND con_hWnd;
static HFONT OemFont;
static LONG OemWidth, OemHeight;
static int ConWidth,ConHeight;
static char szConName[] = "PrBoomConWinClass";
static char Lines[(80+2)*25+1];
static char *Last = NULL;

static char szConTitle[256];
static char szConTitlePrefix[] = "PrBoom Console 2.02 - ";

static CALLBACK
#ifdef CYGWIN /* shut up compiler */
int
#endif
ConWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT paint;
  HDC dc;

  switch (iMsg) {
  case WM_CLOSE:
    return 1;
    break;
  case WM_SYSKEYDOWN:
  case WM_KEYDOWN:
  case WM_SYSKEYUP:
  case WM_KEYUP:
    SendMessage(ghWnd,iMsg,wParam,lParam);
    break;
  case WM_PAINT:
    if ((dc = BeginPaint (con_hWnd, &paint)))  // sf: shut up compiler
      {
	if (Last)
	  {
	    char *row;
	    int line, last;
	    
	    line = paint.rcPaint.top / OemHeight;
	    last = paint.rcPaint.bottom / OemHeight;
	    for (row = Lines + (line*(80+2)); line <= last; line++)
	      {
		TextOut (dc, 0, line * OemHeight, row + 2, row[1]);
		row += 80 + 2;
	      }
	  }
	EndPaint (con_hWnd, &paint);
      }
    return 0;
    break;
    default:
    return(DefWindowProc(hwnd,iMsg,wParam,lParam));
  }

  return 0;
}

static void I_PrintStr (int xp, const char *cp, int count, BOOL scroll)
{
  RECT rect;
  HDC conDC;
  
  if (count)
    {
      conDC=GetDC(con_hWnd);
      TextOut (conDC, xp * OemWidth, ConHeight - OemHeight, cp, count);
      ReleaseDC(con_hWnd,conDC);

      
      if (scroll)
	{
	  rect.left = 0;
	  rect.top = 0;
	  rect.right = ConWidth;
	  rect.bottom = ConHeight;
	  ScrollWindowEx (con_hWnd, 0, -OemHeight, NULL, &rect, NULL, NULL, SW_ERASE|SW_INVALIDATE);
	  UpdateWindow (con_hWnd);
	}
    }
}

static int I_ConPrintString (const char *outline)
{
  const char *cp, *newcp;
  static int xp = 0;
  int newxp;
  BOOL scroll;
  
  cp = outline;
  while (*cp) {
    for (newcp = cp, newxp = xp;
	 *newcp != '\n' && *newcp != '\0' && newxp < 80;
	 newcp++, newxp++) {
      if (*newcp == '\x8') {
	if (xp) xp--;
	newxp = xp;
	cp++;
      }
    }
    
    if (*cp) {
      const char *poop;
      int x;
      
      for (x = xp, poop = cp; poop < newcp; poop++, x++) {
        Last[x+2] = ((*poop) < 32) ? 32 : (*poop);
      }
      
      if (Last[1] < xp + (newcp - cp))
	Last[1] = xp + (newcp - cp);
      
      if (*newcp == '\n' || xp == 80) {
	if (*newcp != '\n') {
	  Last[0] = 1;
	}
	memmove (Lines, Lines + (80 + 2), (80 + 2) * (25 - 1));
	Last[0] = 0;
	Last[1] = 0;
	newxp = 0;
	scroll = true;
      } else {
	scroll = false;
      }
      I_PrintStr (xp, cp, newcp - cp, scroll);
      
      xp = newxp;
      
      if (*newcp == '\n')
	cp = newcp + 1;
      else
	cp = newcp;
    }
  }
  
  return strlen (outline);
}

static void Init_Console(void)
{
  memset(Lines,0,25*(80+2)+1);
	Last = Lines + (25 - 1) * (80 + 2);
}

static int Init_ConsoleWin(HINSTANCE hInstance)
{
  HDC conDC;
  WNDCLASS wndclass;
  TEXTMETRIC metrics;
  RECT cRect;
  int width,height;
  int scr_width,scr_height;
  
  Init_Console();
  /* Register the frame class */
  wndclass.style         = CS_OWNDC;
  wndclass.lpfnWndProc   = (WNDPROC)ConWndProc;
  wndclass.cbClsExtra    = 0;
  wndclass.cbWndExtra    = 0;
  wndclass.hInstance     = hInstance;
  wndclass.hIcon         = LoadIcon (win_hInstance, IDI_WINLOGO);
  wndclass.hCursor       = LoadCursor (NULL,IDC_ARROW);
  wndclass.hbrBackground = (HBRUSH)GetStockObject (BLACK_BRUSH);
  wndclass.lpszMenuName  = szConName;
  wndclass.lpszClassName = szConName;
  
  if (!RegisterClass(&wndclass))
    return false;
  
  width=100;
  height=100;
  con_hWnd = CreateWindow(szConName, szConName, 
			  WS_CAPTION | WS_POPUP,
			  0, 0, width, height,
			  NULL, NULL, hInstance, NULL);
  conDC=GetDC(con_hWnd);
  OemFont = GetStockObject(OEM_FIXED_FONT);
  SelectObject(conDC, OemFont);
  GetTextMetrics(conDC, &metrics);
  OemWidth = metrics.tmAveCharWidth;
  OemHeight = metrics.tmHeight;
  GetClientRect(con_hWnd, &cRect);
  width += (OemWidth * 80) - cRect.right;
  height += (OemHeight * 25) - cRect.bottom;
  // proff 11/09/98: Added code for centering console
  scr_width = GetSystemMetrics(SM_CXFULLSCREEN);
  scr_height = GetSystemMetrics(SM_CYFULLSCREEN);
  MoveWindow(con_hWnd, (scr_width-width)/2, (scr_height-height)/2, width, height, TRUE);
  GetClientRect(con_hWnd, &cRect);
  ConWidth = cRect.right;
  ConHeight = cRect.bottom;
  SetTextColor(conDC, RGB(192,192,192));
  SetBkColor(conDC, RGB(0,0,0));
  SetBkMode(conDC, OPAQUE);
  ReleaseDC(con_hWnd,conDC);
  ShowWindow(con_hWnd, SW_SHOW);
  UpdateWindow(con_hWnd);

  return true;
}

#endif

static void Set_Title(void)
{
  char *p, *pEnd;

// proff 11/06/98: Added setting of console title
  memset(szTitle,0,sizeof(szTitle));
  memcpy(szTitle,szTitlePrefix,strlen(szTitlePrefix));
#ifdef CONSOLE
  memset(szConTitle,0,sizeof(szConTitle));
  memcpy(szConTitle,szConTitlePrefix,strlen(szConTitlePrefix));
#endif
  
  p = title;
  pEnd = p + strlen(title) - 1;
  while (*p == ' ') p++;
  while (*pEnd == ' ') pEnd--;
  pEnd++;
  *pEnd = 0;
  
  if (pEnd>p)
    {
      memcpy(&szTitle[strlen(szTitle)],p,strlen(p));
#ifdef CONSOLE
      memcpy(&szConTitle[strlen(szConTitle)],p,strlen(p));
#endif
    }
  SetWindowText(ghWnd,szTitle);
#ifdef CONSOLE
  SetWindowText(con_hWnd,szConTitle);
#endif
}

static void Init_Dib(void)
{
  View_bmi=malloc(sizeof(BITMAPINFO)+256*4);
  memset(View_bmi,0,40);
  View_bmi->bmiHeader.biSize = 40;
  View_bmi->bmiHeader.biPlanes = 1;
  View_bmi->bmiHeader.biBitCount = 8;
  View_bmi->bmiHeader.biCompression = BI_RGB;
  if (ViewScale==Scale_Own)
    {
      View_bmi->bmiHeader.biWidth = SCREENWIDTH*2;
      View_bmi->bmiHeader.biHeight = SCREENHEIGHT*2;
      ViewMem = malloc((SCREENWIDTH*2)*(SCREENHEIGHT*2));
    }
  else if (ViewScale==Scale_Windows)
    {
      View_bmi->bmiHeader.biWidth = SCREENWIDTH;
      View_bmi->bmiHeader.biHeight = -SCREENHEIGHT;
      ViewMem=NULL;
    }
  else
    {
      // sf: *multiply for hires
      View_bmi->bmiHeader.biWidth = SCREENWIDTH*multiply; 
      View_bmi->bmiHeader.biHeight = -SCREENHEIGHT*multiply;
      ViewMem=NULL;
    }
  ViewDC = GetDC(ghWnd);
  SetStretchBltMode(ViewDC,COLORONCOLOR);
}

static void Init_Mouse(void)
{
  // proff 08/15/98: Made -grabmouse default
  //  if (M_CheckParm("-grabmouse"))
  //    grabMouse=true;
  //    if (M_CheckParm("-nomouse"))
  //      noMouse=true;
  noMouse=(usemouse==0);
}


#ifdef DIRECTX
static void Done_DDraw(void)
{
    if (lpDD)
        IDirectDraw_RestoreDisplayMode(lpDD);
    if (lpDD)
        IDirectDraw_SetCooperativeLevel(lpDD,NULL,DDSCL_NORMAL);
    if (lpDDPal)
        IDirectDrawPalette_Release(lpDDPal);
    if (lpDDPSF)
        IDirectDrawSurface_Release(lpDDPSF);
    if (lpDD)
        IDirectDraw_Release(lpDD);
    lpDDPal=NULL;
    lpDDPSF=NULL;
    lpDD=NULL;
    MoveWindow(ghWnd, 0, 0, MainWinWidth, MainWinHeight, TRUE);
    BringWindowToTop(ghWnd);
}

static void DDrawFullscreen(int fullscreen)
{
  HRESULT error;
  DDSURFACEDESC ddSD;
  DDSCAPS ddSDC;
  int c;
  
  if (fullscreen)
    {
      vidFullScreen = 0;
      error = DirectDrawCreate(NULL,&lpDD,NULL);
      if (error != DD_OK)
	{
	  printf("Error: DirectDrawCreate failed!\n");
	  FullscreenProc=WinFullscreen;
	  Done_DDraw();
	  return;
	}
      error = IDirectDraw_SetCooperativeLevel(lpDD,ghWnd,DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
      if (error != DD_OK)
	{
	  printf("Error: DirectDraw_SetCooperativeLevel failed!\n");
	  FullscreenProc=WinFullscreen;
	  Done_DDraw();
	  return;
	}
      error = IDirectDraw_SetDisplayMode(lpDD,BestWidth,BestHeight,8);
      if (error != DD_OK)
	{
	  printf("Error: DirectDraw_SetDisplayMode %ix%ix8 failed!\n",BestWidth,BestHeight);
	  FullscreenProc=WinFullscreen;
	  Done_DDraw();
	  return;
	}
      else
	printf("DDrawMode %ix%i\n",BestWidth,BestHeight);
      ddSD.dwSize = sizeof(ddSD);
      ddSD.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
      ddSD.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
      ddSD.dwBackBufferCount = 1;
      error = IDirectDraw_CreateSurface(lpDD,&ddSD,&lpDDPSF,NULL);
      if (error != DD_OK)
	{
	  lpDDPSF = NULL;
	  printf("Error: DirectDraw_CreateSurface failed!\n");
	  FullscreenProc=WinFullscreen;
	  Done_DDraw();
	  return;
	}
      ddSDC.dwCaps = DDSCAPS_BACKBUFFER;
      error = IDirectDrawSurface_GetAttachedSurface(lpDDPSF,&ddSDC,&lpDDBSF);
      if (error != DD_OK)
	{
	  lpDDBSF = NULL;
	  printf("Error: DirectDraw_GetAttachedSurface failed!\n");
	  FullscreenProc=WinFullscreen;
	  Done_DDraw();
	  return;
	}
      error = IDirectDraw_CreatePalette(lpDD,DDPCAPS_8BIT | DDPCAPS_ALLOW256,ADDPal,&lpDDPal,NULL);
      if (error != DD_OK)
	{
	  lpDDPal = NULL;
	  printf("Error: DirectDraw_CreatePal failed!\n");
	  FullscreenProc=WinFullscreen;
	  Done_DDraw();
	  return;
	}
      error = IDirectDrawSurface_SetPalette(lpDDPSF,lpDDPal);
      error = IDirectDrawSurface_SetPalette(lpDDBSF,lpDDPal);
      for (c=0; c<256; c++)
	{
	  ADDPal[c].peRed = View_bmi->bmiColors[c].rgbRed;
	  ADDPal[c].peGreen = View_bmi->bmiColors[c].rgbGreen;
	  ADDPal[c].peBlue = View_bmi->bmiColors[c].rgbBlue;
	  ADDPal[c].peFlags = 0;
	}
      IDirectDrawPalette_SetEntries(lpDDPal,0,0,256,ADDPal);
      vidFullScreen = 1;
      printf("Fullscreen-Mode\n");
    }
  else
    {
      //      Done_DDraw();
      if (ViewScale==Scale_Own)
	ViewMemPitch=SCREENWIDTH*2;
      else
	ViewMemPitch=SCREENWIDTH;
      for (c=0; c<256; c++)
	{
	  View_bmi->bmiColors[c].rgbRed = ADDPal[c].peRed;
	  View_bmi->bmiColors[c].rgbGreen = ADDPal[c].peGreen;
	  View_bmi->bmiColors[c].rgbBlue = ADDPal[c].peBlue;
	}
      printf("Windows-Mode\n");
    }
}

static HRESULT WINAPI MyEnumModesCallback(LPDDSURFACEDESC lpDDSDesc, LPVOID lpContext)
{
  int SearchedWidth=SCREENWIDTH;
  int SearchedHeight=SCREENHEIGHT;

  if (ViewScale!=Scale_None)
    {
      SearchedWidth *= 2;
      SearchedHeight *= 2;
    }
  if(hires)
    {
      SearchedWidth *= 2;
      SearchedHeight *= 2;
    }
    
  //  lprintf(LO_INFO,"W: %4i",lpDDSDesc->dwWidth);
  //  lprintf(LO_INFO,", H: %4i",lpDDSDesc->dwHeight);
  //  lprintf(LO_INFO,"\n");
  if (((int)lpDDSDesc->dwWidth>=SearchedWidth) & ((int)lpDDSDesc->dwHeight>=SearchedHeight))
    if ((BestWidth>SearchedWidth) & (BestHeight>SearchedHeight))
    {
      BestWidth=lpDDSDesc->dwWidth;
      BestHeight=lpDDSDesc->dwHeight;
    }
  return DDENUMRET_OK;
}

static void Init_DDraw(void)
{
  HRESULT error;
  DDSURFACEDESC DDSDesc;

  FullscreenProc = DDrawFullscreen;
  error = DirectDrawCreate(NULL,&lpDD,NULL);

  if (error != DD_OK)
    {
      printf("Error: DirectDrawCreate failed!\n");
      FullscreenProc=WinFullscreen;
      return;
    }

  DDSDesc.dwSize=sizeof(DDSURFACEDESC);
  DDSDesc.dwFlags=DDSD_PIXELFORMAT;
  DDSDesc.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
  DDSDesc.ddpfPixelFormat.dwFlags=DDPF_PALETTEINDEXED8 | DDPF_RGB;
  DDSDesc.ddpfPixelFormat.u1.dwRGBBitCount=8;
  BestWidth=INT_MAX;
  BestHeight=INT_MAX;
  IDirectDraw_EnumDisplayModes(lpDD,0,&DDSDesc,NULL,&MyEnumModesCallback);
  if (((BestWidth==INT_MAX) | (BestHeight==INT_MAX)) & (ViewScale!=Scale_None))
    {
      ViewScale=Scale_None;
      BestWidth=INT_MAX;
      BestHeight=INT_MAX;
      IDirectDraw_EnumDisplayModes(lpDD,0,&DDSDesc,NULL,&MyEnumModesCallback);
    }
  if ((BestWidth==INT_MAX) | (BestHeight==INT_MAX))
    {
      BestWidth=0;
      BestHeight=0;
      printf("Error: No DirectDraw mode, which suits the needs, found!\n");
      printf("Searched Mode: W: %i, H: %i, BPP 8\n",SCREENWIDTH,SCREENHEIGHT);
      FullscreenProc=WinFullscreen;
      return;
    }
  //  lprintf(LO_INFO,"BestWidth: %i, BestHeight: %i\n",BestWidth,BestHeight);
  if (lpDD)
    IDirectDraw_Release(lpDD);
  lpDD=NULL;
}
#endif


static boolean Win32_SetMode(int mode)
{
  // proff 08/17/98: Changed for high-res
  
  ViewScale=Scale_None;
  multiply=1;
  ViewMemPitch=SCREENWIDTH;
  vidFullScreen=0;
  hires=0;
  
  switch(mode)
    {
      case 0:        // no multiply
	break;

      case 1:        // stretch by windows x2
	ViewScale=Scale_Windows;
	multiply=2;
	break;

      case 2:        // stretch by windows x3
	ViewScale=Scale_Windows;
	multiply=3;
	break;
	
      case 3:        // internal scaling
	ViewScale=Scale_Own;
	multiply=2;
	ViewMemPitch=SCREENWIDTH*2;
	break; 

      case 4:         // hires
	hires=1;
	multiply=2;
	break;

      case 5:         // dx fullscreen
	vidFullScreen = 1;
	break;

      case 6:         // dx fullscreen/hires
	hires=1;
	vidFullScreen = 1;
	break;
    }

  // proff 08/18/98: This disables the setting of the priority-class
  if (M_CheckParm("-noidle"))
    noidle=true;

  MainWinWidth=SCREENWIDTH;
  MainWinHeight=SCREENHEIGHT;

  ghWnd = CreateWindow(szAppName, szAppName, 
		       WS_CAPTION | WS_POPUP,
	 // proff 06/30/98: Changed form constant value to variable
		       0, 0, MainWinWidth, MainWinHeight,
		       NULL, NULL, win_hInstance, NULL);

  ShowWindow(ghWnd, SW_SHOW);
  UpdateWindow(ghWnd);
  
  // ---- stuff moved from Init_Winstuff

  BringWindowToTop(ghWnd);

  // Set the windowtitle
  Set_Title();

  Init_Dib();
      
  GetClientRect(ghWnd, &ViewRect);
  
  //    lprintf (LO_DEBUG, "I_InitGraphics: Client area: %ux%u\n",
  //            ViewRect.right-ViewRect.left, ViewRect.bottom-ViewRect.top);
  if ( (ViewRect.right-ViewRect.left) != (SCREENWIDTH *multiply) )
    MainWinWidth += (SCREENWIDTH*multiply) - (ViewRect.right -ViewRect.left);
  if ( (ViewRect.bottom-ViewRect.top) != (SCREENHEIGHT*multiply) )
    MainWinHeight+= (SCREENHEIGHT*multiply) - (ViewRect.bottom-ViewRect.top );
  
  MoveWindow(ghWnd, 0, 0, MainWinWidth, MainWinHeight, TRUE);
  GetClientRect(ghWnd, &ViewRect);

  //    lprintf (LO_DEBUG, "I_InitGraphics: Client area: %ux%u\n",
  //            ViewRect.right-ViewRect.left, ViewRect.bottom-ViewRect.top);
  
 
 FullscreenProc = WinFullscreen;

#ifdef DIRECTX
  if (!M_CheckParm("-noddraw"))
    Init_DDraw();
#endif
 
  FullscreenProc(vidFullScreen);

  return true;       // opened ok
}

// sf: close open window

static void Win32_UnsetMode()
{
  if(ghWnd)
    DestroyWindow(ghWnd);
#ifdef DIRECTX
  Done_DDraw();
#endif

}

static void Win32_SetPalette(unsigned char *pal)
{
  int c;
  int col;
  
  if (View_bmi == NULL)
    return;
#ifdef DIRECTX
  if ((vidFullScreen) & (lpDDPal != NULL) & (lpDDBSF != NULL))
    {
      col = 0;
      for (c=0; c<256; c++)
        {
	  ADDPal[c].peRed = gamma_xlate[pal[col++]];
	  ADDPal[c].peGreen = gamma_xlate[pal[col++]];
	  ADDPal[c].peBlue = gamma_xlate[pal[col++]];
	  ADDPal[c].peFlags = 0;
        }
      IDirectDrawPalette_SetEntries(lpDDPal,0,0,256,ADDPal);
    }
  else
#endif
    {
      col = 0;
      for (c=0; c<256; c++)
        {
	  View_bmi->bmiColors[c].rgbRed = gamma_xlate[pal[col++]];
	  View_bmi->bmiColors[c].rgbGreen = gamma_xlate[pal[col++]];
	  View_bmi->bmiColors[c].rgbBlue = gamma_xlate[pal[col++]];
	  View_bmi->bmiColors[c].rgbReserved = 0;
        }
    }
}


static boolean Win32_Init()
{
  // ----- moved from Init_Win ------

  win_hInstance = main_hInstance;
  
  /* Register the frame class */
  wndclass.style         = 0;
  wndclass.lpfnWndProc   = (WNDPROC)WndProc;
  wndclass.cbClsExtra    = 0;
  wndclass.cbWndExtra    = 0;
  wndclass.hInstance     = win_hInstance;
  wndclass.hIcon         = LoadIcon (win_hInstance, IDI_WINLOGO);
  wndclass.hCursor       = LoadCursor (NULL,IDC_ARROW);
  wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wndclass.hbrBackground = NULL;
  wndclass.lpszMenuName  = szAppName;
  wndclass.lpszClassName = szAppName;
  
  if (!RegisterClass(&wndclass))
    return FALSE;

  frameX = GetSystemMetrics(SM_CXFIXEDFRAME);
  frameY = GetSystemMetrics(SM_CYFIXEDFRAME);
  capY = GetSystemMetrics(SM_CYCAPTION);

  // ----- end -----
   
  //  BringWindowToTop(ghWnd);
  Init_Mouse();

    // proff 07/22/98: Added options -fullscr and -nofullscr
  if (M_CheckParm("-fullscr"))
    vidFullScreen=1;
  if (M_CheckParm("-nofullscr"))
    vidFullScreen=0;
  
  return TRUE;
}

static void Win32_Shutdown(void)
{
  if (FullscreenProc)
    FullscreenProc(false);
#ifdef DIRECTX
  Done_DDraw();
#endif
  if (ViewDC)
    ReleaseDC(ghWnd,ViewDC);
  DestroyWindow(ghWnd);
  if (!noMouse)
    {
      ClipCursor(NULL);
    }
}

static void V_ScaleBy2D (void)
{
  unsigned int *olineptrs[2];
  register unsigned int *ilineptr;
  int x, y;
  register unsigned int twoopixels;
  register unsigned int twomoreopixels;
  register unsigned int fouripixels;
  
  ilineptr = (unsigned int *) screens[0];
  olineptrs[0] = (unsigned int *) &ScaledVMem[0];
  olineptrs[1] = (unsigned int *) &ScaledVMem[ViewMemPitch];
  
  // proff 06/30/98: Changed form constant value to defined value
  for (y=SCREENHEIGHT; y>0; y--)
    {
      // proff 06/30/98: Changed form constant value to defined value
      for (x=(SCREENWIDTH/4); x>0; x--)
        {
	  fouripixels = *ilineptr++;
	  twoopixels =    (fouripixels & 0xff000000)
            |    ((fouripixels>>8) & 0xffff00)
            |    ((fouripixels>>16) & 0xff);
	  twomoreopixels =    ((fouripixels<<16) & 0xff000000)
            |    ((fouripixels<<8) & 0xffff00)
            |    (fouripixels & 0xff);
	  *olineptrs[0]++ = twomoreopixels;
	  *olineptrs[0]++ = twoopixels;
	  *olineptrs[1]++ = twomoreopixels;
	  *olineptrs[1]++ = twoopixels;
        }
      olineptrs[0] += ViewMemPitch>>2;
      olineptrs[1] += ViewMemPitch>>2;
    }
}

static void V_ScaleBy2U (void)
{
    unsigned int *olineptrs[2];
    register unsigned int *ilineptr;
    int x, y;
    register unsigned int twoopixels;
    register unsigned int twomoreopixels;
    register unsigned int fouripixels;

// proff 06/30/98: Changed form constant value to defined value
    ilineptr = (unsigned int *) &screens[0][SCREENWIDTH*(SCREENHEIGHT-1)];
    olineptrs[0] = (unsigned int *) &ScaledVMem[0];
    olineptrs[1] = (unsigned int *) &ScaledVMem[ViewMemPitch];

// proff 06/30/98: Changed form constant value to defined value
    for (y=SCREENHEIGHT; y>0; y--)
    {
// proff 06/30/98: Changed form constant value to defined value
        for (x=(SCREENWIDTH/4); x>0; x--)
        {
        fouripixels = *ilineptr++;
        twoopixels =    (fouripixels & 0xff000000)
            |    ((fouripixels>>8) & 0xffff00)
            |    ((fouripixels>>16) & 0xff);
        twomoreopixels =    ((fouripixels<<16) & 0xff000000)
            |    ((fouripixels<<8) & 0xffff00)
            |    (fouripixels & 0xff);
        *olineptrs[0]++ = twomoreopixels;
        *olineptrs[0]++ = twoopixels;
        *olineptrs[1]++ = twomoreopixels;
        *olineptrs[1]++ = twoopixels;
        }
// proff 06/30/98: Changed form constant value to defined value
        ilineptr -= (SCREENWIDTH/2);
        olineptrs[0] += ViewMemPitch>>2;
        olineptrs[1] += ViewMemPitch>>2;
    }
}

#ifndef NOASM
void V_ScaleBy2Da(void);
void V_ScaleBy2Ua(void);
#endif //NOASM

static void Win32_FinishUpdate(void)
{
#ifdef DIRECTX
  HRESULT error;
  DDSURFACEDESC ddSD;
  int y;
  static boolean SurfaceWasLost=true;
  char *Surface,*doommem;
  
  if ((vidFullScreen) & (lpDDPSF != NULL) & (lpDDBSF != NULL))
    {
      ddSD.dwSize = sizeof(ddSD);
      ddSD.dwFlags = 0;
      error = IDirectDrawSurface_Lock(lpDDBSF,NULL,&ddSD,DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT,NULL);
      if (error == DDERR_SURFACELOST)
        {
	  SurfaceWasLost=true;
	  IDirectDrawSurface_Restore(lpDDPSF);
	  IDirectDrawSurface_Restore(lpDDBSF);
	  return;
        }
      if (error != DD_OK)
	return;
      if (SurfaceWasLost)
        {
	  Surface = ddSD.lpSurface;
	  for (y=0; (DWORD)y<ddSD.dwHeight; y++)
            {
	      memset(Surface,0,ddSD.dwWidth);
	      Surface += ddSD.u1.lPitch;
            }
        }

      if (1) //ViewScale==Scale_None)
        {  
	  int wid = SCREENWIDTH << hires;
	  int hi = SCREENHEIGHT << hires;

	  Surface = 
	    (char *)ddSD.lpSurface + 
	    ((ddSD.dwWidth - wid) / 2) +
	    (((ddSD.dwHeight - hi) / 2) * ddSD.u1.lPitch);

	  doommem = screens[0];

	  // sf: speedup by removing loop if possible

	  if(wid == ddSD.u1.lPitch)
	    memcpy(Surface, doommem, wid * hi);
	  else
	    {
	      for (y=0; y<hi; y++)
		{
		  memcpy(Surface, doommem, wid);
		  Surface += ddSD.u1.lPitch;
		  doommem += wid;
		}
	    }
        }
      else
        {
	  ViewMemPitch = ddSD.u1.lPitch;
	  ScaledVMem = 
	    (char *)ddSD.lpSurface + 
	    ((ddSD.dwWidth / 2) - SCREENWIDTH) +
	    ((ddSD.dwHeight / 2) - SCREENHEIGHT) * ddSD.u1.lPitch;
	    
#ifndef NOASM
	  V_ScaleBy2Da();
#else
	  V_ScaleBy2D();
#endif
        }

      error = IDirectDrawSurface_Unlock(lpDDBSF,ddSD.lpSurface);
      if (error != DD_OK)
	return;
      error = IDirectDrawSurface_Flip(lpDDPSF,NULL,DDFLIP_WAIT);
      if (error != DD_OK)
	return;
    }
  else
#endif
    {
      if (ViewScale==Scale_None)
	{
	  // sf: multiply for hires
	  if(SetDIBitsToDevice(ViewDC, 0, 0,
			       SCREENWIDTH*multiply, SCREENHEIGHT*multiply,
			       0, 0, 0, SCREENHEIGHT*multiply,
			       screens[0], View_bmi, DIB_RGB_COLORS)
	     == GDI_ERROR)
	    I_Error("SetDIBitsToDevice failed");
	}
      else if (ViewScale==Scale_Windows)
	{
	  // sf: generalisd stretch: *3, *4 etc are possible
	  if (StretchDIBits(ViewDC, 0, 0,
			    SCREENWIDTH*multiply, SCREENHEIGHT*multiply,
			    0, 0, SCREENWIDTH, SCREENHEIGHT,
			    screens[0], View_bmi, DIB_RGB_COLORS, SRCCOPY)
	      == GDI_ERROR )
	    I_Error("StretchDIBits failed");
	}
      else if (ViewScale==Scale_Own)
	{
	  ScaledVMem=ViewMem;
#ifndef NOASM
	  V_ScaleBy2Ua();
#else
	  V_ScaleBy2U();
#endif
	  if (SetDIBitsToDevice(ViewDC, 0, 0, SCREENWIDTH*2, SCREENHEIGHT*2,
				0, 0, 0, SCREENHEIGHT*2,
				ScaledVMem, View_bmi, DIB_RGB_COLORS)
	      == GDI_ERROR )
	    I_Error("SetDIBitsToDevice failed");
	}
      GdiFlush();    
    }
}

static void Win32_StartTic(void)
{
  MSG msg;
  POINT point;
  static LONG prevX=0, prevY=0;
  static int hadMouse = 0;
  event_t event;
  RECT rectw;
  
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
    //    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  
  if (!noMouse)
    {
      if(!mouse_grabbed && ((grabnow && fActive) || vidFullScreen))
	{
	  RECT rect;
	  RECT rectc;
	  RECT rectw;

	  ClipCursor(NULL);
	  GetWindowRect(ghWnd, &rectw);
	  GetClientRect(ghWnd, &rectc);
	  rect.left = rectw.left + frameX;
	  rect.top = rectw.top + frameY + capY;
	  rect.right = rect.left + (rectc.right - rectc.left);
	  rect.bottom = rect.top + (rectc.bottom - rectc.top);
	  ClipCursor(&rect);
	  ShowCursor(FALSE);
	  mouse_grabbed = true;
	}
      if(mouse_grabbed && !((grabnow && fActive) || vidFullScreen))
	{
	  ClipCursor(NULL);
	  ShowCursor(TRUE);
	  mouse_grabbed = false;
	}
      
      if ( !GetCursorPos(&point) )
	I_Error("GetCursorPos() failed");
      /*      if (hadMouse && fActive)*/
      if(fActive)
	{
	  if ( (prevX != point.x) || (prevY != point.y) )
	    {
	      event.type = ev_mouse;
	      event.data1 = 0;
	      event.data2 = ((point.x - prevX))/2;
	      event.data3 = ((prevY - point.y))/2;
	      D_PostEvent(&event);

	      prevX = point.x;
	      prevY = point.y;
	    }
	  
	  if(grabnow || vidFullScreen)
	    {
	      GetWindowRect(ghWnd, &rectw);
	      prevX = (rectw.left + rectw.right) / 2;
	      prevY = (rectw.top + rectw.bottom) / 2;
	      if ( !SetCursorPos(prevX,prevY) )
		I_Error("SetCursorPos() failed");
	    }
	}
      else
	{
	  prevX = point.x;
	  prevY = point.y;
	  hadMouse = 1;
	}
    }
}

static void Win32_StartFrame()
{
  // heh
}

//===========================================================================
//
// Modes / Driver
//
//===========================================================================

static char *win32_modenames[] =
  {
    "320x200",
    "320x200 stretched x2 (windows)",
    "320x200 stretched x3 (windows)",
    "320x200 stretched x2 (internal)",
    "640x400",

    "320x200 Fullscreen",
    "640x400 Fullscreen",

    NULL
  };

viddriver_t win32_driver =
  {
    "windows",
    "-win",
    Win32_Init,
    Win32_Shutdown,

    Win32_SetMode,
    Win32_UnsetMode,
    
    Win32_FinishUpdate,
    Win32_SetPalette,

    Win32_StartTic,
    Win32_StartFrame,

    win32_modenames,
  };
