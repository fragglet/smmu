// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Misc Video stuff.
//
// Font. Loading box. FPS ticker, etc
//
//---------------------------------------------------------------------------

#include <stdio.h>
#include "c_io.h"
#include "c_runcmd.h"
#include "doomdef.h"
#include "doomstat.h"
#include "i_video.h"
#include "v_video.h"
#include "v_misc.h"
#include "w_wad.h"

extern int gamma_correct;

/******************************
   CONSOLE VIDEO MODE COMMANDS
 ******************************/
// non platform-specific stuff is here in v_misc.c
// platform-specific stuff is in i_video.c
// videomode_t is platform specific although it must
// contain a member of type char* called description:
// see i_video.c for more info

int v_mode = 0;
int prevmode = 0;

int NumModes()
{
  int count=0;

  while(videomodes[count].description)
    count++;

  return count;
}

// v_resetmode is called after changing vid mode

void V_ResetMode()
{
  // check for invalid mode

  if(v_mode >= NumModes() || v_mode < 0)
    {
      C_Printf("invalid mode %i", v_mode);
      v_mode = prevmode;
      return;
    }
  
  prevmode = v_mode;
  
  I_SetMode(v_mode);
}

/******************
         FONT STUFF
 ******************/

patch_t* v_font[V_FONTSIZE];
patch_t *bgp[9];        // background for boxes

void V_LoadFont()
{
  int i, j;
  char tempstr[10];

  // init to NULL first
  for(i=0; i<V_FONTSIZE; i++)
    v_font[i] = NULL;

  for(i=0, j=V_FONTSTART; i<V_FONTSIZE; i++, j++)
    {
      if(j>96 && j!=121 && j!=123 && j!=124 && j!=125) continue;
      sprintf(tempstr, "STCFN%.3d",j);
      v_font[i] = W_CacheLumpName(tempstr, PU_STATIC);
    }
}

 // sf: write a text line to x, y
void V_WriteText(unsigned char *s, int x, int y)
{
  int   w;
  unsigned char* ch;
  char *colour = cr_red;
  unsigned int c;
  int   cx;
  int   cy;
  patch_t *patch;

  ch = s;
  cx = x;
  cy = y;
  
  while(1)
    {
      c = *ch++;
      if (!c)
	break;
      if (c >= 128)     // new colour
      {
           colour = colrngs[c - 128];
           continue;
      }
      if (c == '\t')
        {
          cx = (cx/40)+1;
          cx = cx*40;
        }
      if (c == '\n')
	{
	  cx = x;
          cy += 8;
	  continue;
	}
      
      c = toupper(c) - V_FONTSTART;
      if (c < 0 || c>= V_FONTSIZE)
	{
	  cx += 4;
	  continue;
	}

      patch = v_font[c];
      if(!patch) continue;

      w = SHORT (patch->width);
      if (cx+w > SCREENWIDTH)
	break;

      V_DrawPatchTranslated(cx, cy, 0, patch, colour, 0);

      cx+=w;
    }
}

// write text in a particular colour

void V_WriteTextColoured(unsigned char *s, int colour, int x, int y)
{
  static char *tempstr = NULL;
  static int allocedsize=0;

  // if string bigger than allocated, realloc bigger
  if(strlen(s) > allocedsize)
    {
      if(tempstr)       // already alloced?
        tempstr = Z_Realloc(tempstr, strlen(s) + 3, PU_STATIC, 0);
      else
        tempstr = Z_Malloc(strlen(s) + 3, PU_STATIC, 0);
      
      allocedsize = strlen(s);  // save for next time
    }
  
  sprintf(tempstr, "%c%s", 128+colour, s);
  
  V_WriteText(tempstr, x, y);
}

// find height(in pixels) of a string 

int V_StringHeight(unsigned char *s)
{
  int height = 8;  // always at least 8

  // add an extra 8 for each newline found

  while(*s)
    {
      if(*s == '\n') height += 8;
      s++;
    }

  return height;
}

int V_StringWidth(unsigned char *s)
{
  int length = 0; // current line width
  int longest_width = 0; // line with longest width so far
  unsigned char c;
  
  for(; *s; s++)
    {
      c = *s;
      if(c >= 128)         // colour
	continue;
      if(c == '\n')        // newline
	{
	  if(length > longest_width) longest_width = length;
	  length = 0; // next line;
	  continue;	  
	}
      c = toupper(c) - V_FONTSTART;
      length += c >= V_FONTSIZE ? 4 : SHORT(v_font[c]->width);
    }

  if(length > longest_width) longest_width = length; // check last line

  return longest_width;
}


/*********************
           BOX DRAWING
 *********************/

#define FG 0

void V_DrawBox(int x, int y, int w, int h)
{
  int xs = bgp[0]->width;
  int ys = bgp[0]->height;
  int i,j;

  // top rows
  V_DrawPatchDirect(x, y, FG, bgp[0]);    // ul
  for (j = x+xs; j < x+w-xs; j += xs)     // uc
    V_DrawPatchDirect(j, y, FG, bgp[1]);
  V_DrawPatchDirect(j, y, FG, bgp[2]);    // ur

  // middle rows
  for (i=y+ys;i<y+h-ys;i+=ys)
    {
      V_DrawPatchDirect(x, i, FG, bgp[3]);    // cl
      for (j = x+xs; j < x+w-xs; j += xs)     // cc
        V_DrawPatchDirect(j, i, FG, bgp[4]);
      V_DrawPatchDirect(j, i, FG, bgp[5]);    // cr
    }

  // bottom row
  V_DrawPatchDirect(x, i, FG, bgp[6]);    // ll
  for (j = x+xs; j < x+w-xs; j += xs)     // lc
    V_DrawPatchDirect(j, i, FG, bgp[7]);
  V_DrawPatchDirect(j, i, FG, bgp[8]);    // lr
}

void V_InitBox()
{
  bgp[0] = (patch_t *) W_CacheLumpName("BOXUL", PU_STATIC);
  bgp[1] = (patch_t *) W_CacheLumpName("BOXUC", PU_STATIC);
  bgp[2] = (patch_t *) W_CacheLumpName("BOXUR", PU_STATIC);
  bgp[3] = (patch_t *) W_CacheLumpName("BOXCL", PU_STATIC);
  bgp[4] = (patch_t *) W_CacheLumpName("BOXCC", PU_STATIC);
  bgp[5] = (patch_t *) W_CacheLumpName("BOXCR", PU_STATIC);
  bgp[6] = (patch_t *) W_CacheLumpName("BOXLL", PU_STATIC);
  bgp[7] = (patch_t *) W_CacheLumpName("BOXLC", PU_STATIC);
  bgp[8] = (patch_t *) W_CacheLumpName("BOXLR", PU_STATIC);
}



/***********************
        'LOADING' PIC
 ***********************/

int loading_amount;
int loading_total;
char *loading_message;

void V_DrawLoading()
{
  int x, y;
  char *dest;
  int linelen;
  
  V_DrawBox((SCREENWIDTH/2)-50, (SCREENHEIGHT/2)-30, 100, 40);
  
  V_WriteText(loading_message, (SCREENWIDTH/2)-30, (SCREENHEIGHT/2)-20);
  
  x = ((SCREENWIDTH/2)-45);
  y = (SCREENHEIGHT/2);
  dest = screens[0] + ((y<<hires)*(SCREENWIDTH<<hires)) + (x<<hires);
  linelen = (90*loading_amount) / loading_total;

  // white line
  memset(dest, 4, linelen<<hires);
  // black line (unfilled)
  memset(dest+(linelen<<hires), 0, (90-linelen)<<hires);

  if(hires)
    {
      dest += SCREENWIDTH<<hires;
      memset(dest, 4, linelen<<hires);
      memset(dest+(linelen<<hires), 0, (90-linelen)<<hires);
    }
  
  I_FinishUpdate();
}

void V_SetLoading(int total, char *mess)
{
  loading_total = total ? total : 1;
  loading_amount = 0;
  loading_message = mess;
  if(in_textmode)
    {
      int i;
      printf(" %s ", mess);
      putchar('[');
      for(i=0; i<total; i++) putchar(' ');     // gap
      putchar(']');
      for(i=0; i<=total; i++) putchar('\b');    // backspace
    }
  else
    V_DrawLoading();
}

void V_LoadingIncrease()
{
  loading_amount++;
  if(in_textmode)
    {
      putchar('.');
      if(loading_amount == loading_total) putchar('\n');
    }
  else
    V_DrawLoading();
}

void V_LoadingSetTo(int amount)
{
  loading_amount = amount;
  if(!in_textmode) V_DrawLoading();
}

/************************
   FPS Ticker
*************************/

// show dots at the bottom of the screen which represent
// an approximation to the current fps of doom.
// moved from i_video.c to make it a bit more
// system non-specific

#define BLACK 0
#define WHITE 4
#define FPS_HISTORY 80
#define CHART_HEIGHT 40
#define X_OFFSET 20
#define Y_OFFSET 20

int v_ticker = false;
static int history[FPS_HISTORY];
int current_count = 0;

void V_ClassicFPSDrawer();

void V_FPSDrawer()
{
  int i;
  int x,y;          // screen x,y
  int cx, cy;       // chart x,y

  if(v_ticker == 2)
    {
      V_ClassicFPSDrawer();
      return;
    }
  
  current_count++;
 
  // render the chart
  for(cx=0, x = X_OFFSET; cx<FPS_HISTORY; x++, cx++)
    for(cy=0, y = Y_OFFSET; cy<CHART_HEIGHT; y++, cy++)
      {
	i = cy > (CHART_HEIGHT-history[cx]) ? BLACK : WHITE;
	screens[0][y*(SCREENWIDTH<<hires) +x] = i;
      }
}

void V_FPSTicker()
{
  static int lasttic;
  int thistic;
  int i;

  thistic = I_GetTime()/7;
  
  if(lasttic != thistic)
    {
      lasttic = thistic;
      
      for(i=0; i<FPS_HISTORY-1; i++)
	history[i] = history[i+1];
      
      history[FPS_HISTORY-1] = current_count;
      current_count = 0;
    }
}

// sf: classic fps ticker kept seperate

void V_ClassicFPSDrawer()
{
  static int lasttic;
  byte *s = screens[0];
  
  int i = I_GetTime();
  int tics = i - lasttic;
  lasttic = i;
  if (tics > 20)
    tics = 20;

  if (hires)    // killough 11/98: hires support
    {           // sf: rewritten so you can distinguish between dots
      
      for(i=0 ; i<tics; i++)
	s[(SCREENHEIGHT*2-1)*SCREENWIDTH*2 + i*4] =
	  s[(SCREENHEIGHT*2-1)*SCREENWIDTH*2 + i*4 + 1] = 0xff;
      for(; i<20; i++)
	s[(SCREENHEIGHT*2-1)*SCREENWIDTH*2 + i*4] =
	  s[(SCREENHEIGHT*2-1)*SCREENWIDTH*2 + i*4 + 1] = 0x0;
    }
  else
    {
      for (i=0 ; i<tics*2 ; i+=2)
	s[(SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
      for ( ; i<20*2 ; i+=2)
	s[(SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
    }
}


//
// V_Init
//
// Allocates the 4 full screen buffers in low DOS memory
// No return value
//
// killough 11/98: rewritten to support hires


void V_Init(void)
{
  int size = hires ? SCREENWIDTH*SCREENHEIGHT*4 : SCREENWIDTH*SCREENHEIGHT;
  static byte *s;

#ifdef DJGPP
  if (s)
    free(s), destroy_bitmap(screens0_bitmap);
#endif

  screens[3] = (screens[2] = (screens[1] = s = calloc(size,3)) + size) + size;

#ifdef DJGPP
  screens0_bitmap = 
    create_bitmap_ex(8, SCREENWIDTH << hires, SCREENHEIGHT << hires);
  memset(screens[0] = screens0_bitmap->line[0], 0, size);
#else
  screens[0] = malloc(size);
#endif

}


void V_InitMisc()
{
  V_LoadFont();
  V_InitBox();
}

/*************************
          CONSOLE COMMANDS
 *************************/

VARIABLE_INT(v_mode, NULL,              0, 10, NULL);

char *str_ticker[]={"off", "chart", "classic"};
VARIABLE_INT(v_ticker, NULL,            0, 2, str_ticker);

CONSOLE_VARIABLE(v_mode, v_mode, cf_buffered)
{
  V_ResetMode();
}

CONSOLE_COMMAND(v_modelist, 0)
{
  videomode_t* videomode = videomodes;
  
  C_Printf(FC_GRAY "video modes:\n" FC_RED);
  
  while(videomode->description)
    {
      C_Printf("%i: %s\n",(int)(videomode-videomodes),
	       videomode->description);
      videomode++;
    }
}

CONSOLE_VARIABLE(v_ticker, v_ticker, 0) {}

void V_AddCommands()
{
  C_AddCommand(v_mode);
  C_AddCommand(v_modelist);
  C_AddCommand(v_ticker);
}
