// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Heads up frag counter
//
// Counts the frags by each player and sorts them so that the best
// player is at the top of the list
//
// By Simon Howard
//
//----------------------------------------------------------------------------

/* includes ************************/

#include <stdio.h>

#include "hu_frags.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "d_player.h"
#include "doomdef.h"
#include "doomstat.h"
#include "g_game.h"
#include "r_draw.h"
#include "w_wad.h"
#include "v_video.h"
#include "z_zone.h"

#define FRAGSX 125
#define FRAGSY 10

#define NAMEX 165
#define NAMEY 65

#define FRAGNUMX 175

/* externs ************************/

extern boolean gamekeydown[NUMKEYS];

/* globals ************************/

player_t *sortedplayers[MAXPLAYERS];

int num_players;
int show_scores;                // enable scores

static patch_t *fragspic;
static patch_t *fragbox;

/* functions ************************/

void HU_FragsInit()
{
  fragspic = W_CacheLumpName("HU_FRAGS", PU_STATIC);
  fragbox = W_CacheLumpName("HU_FRGBX", PU_STATIC);
}

void HU_FragsDrawer()
{
  int i, y;
  char tempstr[50];
  
  if(((players[displayplayer].playerstate!=PST_DEAD || walkcam_active)
      && !gamekeydown[key_frags]) || !deathmatch )
    return;
  
  if(!show_scores) return;
  
  // "frags"
  V_DrawPatch(FRAGSX, FRAGSY, 0, fragspic);
  
  y = NAMEY;
  
  for(i=0; i<num_players; i++)
    {
      // write their name
      sprintf(tempstr, "%s%s", !demoplayback && 
	      sortedplayers[i]==players+consoleplayer ? FC_GRAY : FC_RED,
	      sortedplayers[i]->name);
      
      V_WriteText(tempstr, NAMEX - V_StringWidth(tempstr), y);
      
      // box behind frag pic
      
      V_DrawPatchTranslated
	(
	 FRAGNUMX, y,
	 0,
	 fragbox,
	 sortedplayers[i]->colormap ?
	      (char*)translationtables+256*(sortedplayers[i]->colormap-1) :
	      cr_red,
	 13
	 );

                        // draw the frags
      sprintf(tempstr, "%i", sortedplayers[i]->totalfrags);
      V_WriteText(tempstr, FRAGNUMX + 16 - V_StringWidth(tempstr)/2, y);
      y += 10;
    }
}

void HU_FragsUpdate()
{
  int i,j;
  int change;
  player_t *temp;
  
  num_players = 0;
  
  for(i=0; i<MAXPLAYERS; i++)
    {
      if(!playeringame[i]) continue;
      
      // found a real player
      // add to list
      
      sortedplayers[num_players] = &players[i];
      num_players++;

      players[i].totalfrags = 0; // reset frag count

      for(j=0; j<MAXPLAYERS; j++)  // add all frags for this player
	{
	  if(!playeringame[j]) continue;
	  if(i==j) players[i].totalfrags-=players[i].frags[j];
	  else players[i].totalfrags+=players[i].frags[j];
	}
    }

  // use the bubble sort algorithm to sort the players
  
  change = true;
  while(change)
    {
      change = false;
      for(i=0; i<num_players-1; i++)
	{
	  if(sortedplayers[i]->totalfrags <
	     sortedplayers[i+1]->totalfrags)
	    {
	      temp = sortedplayers[i];
	      sortedplayers[i] = sortedplayers[i+1];
	      sortedplayers[i+1] = temp;
	      change = true;
	    }
	}
    }
}

void HU_FragsErase()
{
  int i;
  
  if(!deathmatch)
    return;
  
  for(i=FRAGSY; i<SCREENHEIGHT-ST_HEIGHT; i++)
    R_VideoErase(i*SCREENWIDTH, SCREENWIDTH);
}

CONSOLE_COMMAND(frags, 0)
{
  int i;
  
  for(i=0; i<num_players; i++)
    {
      C_Printf(FC_GRAY"%i"FC_RED" %s\n",
	       sortedplayers[i]->totalfrags,
	       sortedplayers[i]->name);
    }
}

VARIABLE_BOOLEAN(show_scores,       NULL,           onoff);
CONSOLE_VARIABLE(show_scores,   show_scores,    0)      {}

void HU_FragsAddCommands()
{
  C_AddCommand(frags);
  C_AddCommand(show_scores);
}

