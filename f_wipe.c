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
// Mission begin melt/wipe screen special effect.
//
// Rewritten by Simon Howard
// Portions which deal with the movement of the columns adapted
// from the original sources
//
//-----------------------------------------------------------------------------

// 13/12/99: restored movement of columns to being the same as in the
// original, while retaining the new 'engine'

#include "c_io.h"
#include "doomdef.h"
#include "d_main.h"
#include "v_video.h"
#include "m_random.h"
#include "f_wipe.h"

// array of pointers to the
// column data for 'superfast' melt
static char *start_screen[MAX_SCREENWIDTH] = {0};

// y co-ordinate of various columns
static int worms[SCREENWIDTH];

#define wipe_scrheight (SCREENHEIGHT<<hires)
#define wipe_scrwidth (SCREENWIDTH<<hires)

boolean        inwipe = false;
static int     starting_height;

void Wipe_Initwipe()
{
  int x;
  
  inwipe = true;
  
  starting_height = current_height<<hires;       // use console height
  
  worms[0] = starting_height - M_Random()%16;

  for(x=1; x<SCREENWIDTH; x++)
    {
      int r = (M_Random()%3) - 1;
      worms[x] = worms[x-1] + r;
      if (worms[x] > 0)
        worms[x] = 0;
      else
        if (worms[x] == -16)
          worms[x] = -15;
    }
}

void Wipe_StartScreen()
{
  int x, y;

  Wipe_Initwipe();
  
  if(!start_screen[0])
    {
      int x;
      for(x=0;x<MAX_SCREENWIDTH;x++)
	start_screen[x] = Z_Malloc(MAX_SCREENHEIGHT,PU_STATIC,0);
    }

  for(x=0; x<wipe_scrwidth; x++)
    {
      // limit check
      int wormy = worms[x >> hires] > 0 ? worms[x >> hires] : 0; 
      
      for(y=0; y<wipe_scrheight-wormy; y++)
	*(start_screen[x] + y) =
	  *(screens[0] + (y+wormy) * wipe_scrwidth + x);
    }
  
  return;
}

void Wipe_Drawer()
{
  int x;
  
  for(x=0; x<wipe_scrwidth; x++)
    {
      char *dest;
      char *src;
      int wormy, y;
      
      wormy = worms[x >> hires] > 0 ? worms[x >> hires] : 0;  // limit check
	  
      wormy <<= hires;
      src = start_screen[x];
      dest = screens[0] + wipe_scrwidth*wormy + x;
      
      for(y=wormy; y<wipe_scrheight; y++)
	{
	  *dest = *src;
	  dest += wipe_scrwidth; src++;
	}
    }
 
  redrawsbar = true; // clean up status bar
}

void Wipe_Ticker()
{
  boolean done;
  int x;
  
  done = true;  // default to true
  
    for (x=0; x<SCREENWIDTH; x++)
      if (worms[x]<0)
        {
          worms[x]++;
          done = false;
        }
      else
        if (worms[x] < wipe_scrheight)
          {
            int dy;

            dy = (worms[x] < 16) ? worms[x]+1 : 8;
            if (worms[x]+dy >= wipe_scrheight)
              dy = wipe_scrheight - worms[x];
            worms[x] += dy;
            done = false;
          }
  
  if(done)
    inwipe = false;
}

