// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: f_wipe.c,v 1.3 1998/05/03 22:11:24 killough Exp $
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
//      Mission begin melt/wipe screen special effect.
//
//-----------------------------------------------------------------------------

// rewritten by fraggle =p

static const char rcsid[] = "$Id: f_wipe.c,v 1.3 1998/05/03 22:11:24 killough Exp $";

#include "c_io.h"
#include "doomdef.h"
#include "d_main.h"
#include "i_video.h"
#include "v_video.h"
#include "m_random.h"
#include "f_wipe.h"

// array of pointers to the
// column data for 'superfast' melt
char *start_screen[MAX_SCREENWIDTH] = {0};

// worm y 
int worms[MAX_SCREENWIDTH];

#define wipe_scrheight (SCREENHEIGHT<<hires)
#define wipe_scrwidth (SCREENWIDTH<<hires)

int            wipe_speed = 12;
boolean        inwipe = false;
boolean        syncmove = false;
int            starting_height;

void Wipe_Initwipe()
{
  int x;
  
  inwipe = true;
  
  starting_height = current_height<<hires;       // use console height
  
  for(x=0; x<wipe_scrwidth; x++)
    {
      worms[x] = starting_height;
    }
  
  syncmove = false;
}

void Wipe_StartScreen()
{
  Wipe_Initwipe();
  
  if(!start_screen[0])
    {
      int x;
      for(x=0;x<MAX_SCREENWIDTH;x++)
	start_screen[x] = Z_Malloc(MAX_SCREENHEIGHT,PU_STATIC,0);
    }
  
  {
    int x, y;
    for(x=0; x<wipe_scrwidth; x++)
      for(y=0; y<wipe_scrheight-worms[x]; y++)
	*(start_screen[x] + y) =
	  *(screens[0] + (y+worms[x]) * wipe_scrwidth + x);
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
      int y;
      
      src = start_screen[x];
      dest = screens[0] + wipe_scrwidth*worms[x] + x;
      
      for(y=worms[x]; y<wipe_scrheight; y++)
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
  boolean keepsyncmove = false; // start moving random, then move together
  int moveamount = 0;
  
  done = true;  // default to true
  
  for(x=0; x<wipe_scrwidth; x++)
    {
      moveamount += (M_Random()%5) - 2;
      if(moveamount < 0) moveamount = 0;
      
      if(worms[x] < wipe_scrheight)
	{                // move the worm down
	  int dy;
	  
	  dy = syncmove ? 12 : moveamount;
	  dy = (dy * wipe_speed) / 12;

	  worms[x] += dy << hires;

	  // move together after a certain amount
	  if(worms[x] > 20+starting_height) keepsyncmove = true; 

	  done = false; // not yet finished
	}
    }
  
  if(done)
    inwipe = false;
  
  syncmove = keepsyncmove;
}

