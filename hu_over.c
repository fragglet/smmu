// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Heads up 'overlay' for fullscreen
//
// Rewritten and put in a seperate module(seems sensible)
//
// By Simon Howard
//
//----------------------------------------------------------------------------

/* includes ************************/

#include <stdio.h>

#include "doomdef.h"
#include "doomstat.h"
#include "c_runcmd.h"
#include "d_deh.h"
#include "d_event.h"
#include "g_game.h"
#include "hu_frags.h"
#include "hu_over.h"
#include "hu_stuff.h"
#include "p_info.h"
#include "p_map.h"
#include "p_setup.h"
#include "r_draw.h"
#include "s_sound.h"
#include "v_video.h"
#include "w_wad.h"

/* defines *******************/

   // internal for other defines:
#define _wc_pammo(w)    ( weaponinfo[w].ammo == am_noammo ? 0 : \
                          players[displayplayer].ammo[weaponinfo[w].ammo])
#define _wc_mammo(w)    ( weaponinfo[w].ammo == am_noammo ? 0 : \
                          players[displayplayer].maxammo[weaponinfo[w].ammo])

   // the colour a weapon should be according to the ammo left
#define weapcolour(w)   ( !_wc_mammo(w) ? *FC_GRAY :    \
        _wc_pammo(w) < ( (_wc_mammo(w) * ammo_red) / 100) ? *FC_RED :    \
        _wc_pammo(w) < ( (_wc_mammo(w) * ammo_yellow) / 100) ? *FC_GOLD : \
                          *FC_GREEN );
   // the amount of ammo the displayplayer has left 
#define playerammo      \
        _wc_pammo(players[displayplayer].readyweapon)
   // the maximum amount the player could have for current weapon
#define playermaxammo   \
        _wc_mammo(players[displayplayer].readyweapon)

    // set up an overlay_t
#define setol(o,a,b)          \
        {                     \
        overlay[o].x = (a);   \
        overlay[o].y = (b);   \
        }
#define HUDCOLOUR FC_GRAY

/* globals ****************************/

int hud_overlaystyle = 1;
int hud_enabled = 1;
int hud_hidestatus = 0;

/********************************
  Heads up font
 ********************************/

// the utility code to draw the tiny heads-up
// font. Based on the code from v_video.c
// which draws the normal font.

// note to programmers from other ports: hu_font is the heads up font
// *not* the general doom font as it is in the original sources and
// most ports

patch_t* hu_font[HU_FONTSIZE];
boolean hu_fontloaded = false;

void HU_LoadFont()
{
  int i, j;
  char lumpname[10];

  for(i=0, j=HU_FONTSTART; i<HU_FONTSIZE; i++, j++)
    {
      lumpname[0] = 0;
      if( (j>='0' && j<='9') || (j>='A' && j<='Z') )
	sprintf(lumpname, "DIG%c", j);
      if(j==45 || j==47 || j==58 || j==91 || j==93)
	sprintf(lumpname, "DIG%i", j);
      if(j>=123 && j<=127)
	sprintf(lumpname, "STBR%i", j);
      if(j=='_') strcpy(lumpname, "DIG45");
      if(j=='(') strcpy(lumpname, "DIG91");
      if(j==')') strcpy(lumpname, "DIG93");
      
      hu_font[i] = lumpname[0] ? W_CacheLumpName(lumpname, PU_STATIC) : NULL;
    }
  hu_fontloaded = true;
}

// sf: write a text line to x, y

void HU_WriteText(unsigned char *s, int x, int y)
{
  int   w;
  unsigned char* ch;
  char *colour = cr_red;
  unsigned int c;
  int   cx;
  int   cy;

  if(!hu_fontloaded) return;
  
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
      if (c == '\n')
	{
	  cx = x;
          cy += 8;
	  continue;
	}
  
      c = toupper(c) - HU_FONTSTART;

      if (c < 0 || c > HU_FONTSIZE || !hu_font[c])
	{
	  cx += 4;
	  continue;
	}

      w = SHORT (hu_font[c]->width);
      if (cx+w > SCREENWIDTH)
	break;

      V_DrawPatchTranslated(cx, cy, 0, hu_font[c], colour, 0);

      cx+=w;
    }
}

        // the width in pixels of a string in heads-up font
int HU_StringWidth(unsigned char *s)
{
  int length = 0;
  unsigned char c;
  
  for(; *s; s++)
    {
      c = *s;
      if(c >= 128)         // colour
	continue;
      c = toupper(c);
      length +=
	c >= HU_FONTSIZE || !hu_font[c] 
	? SHORT(hu_font[c - HU_FONTSTART]->width) : 4;
    }
  return length;
}

#define BARSIZE 15

        // create a string containing the text 'bar' which graphically
        // show %age of ammo/health/armor etc left

void HU_TextBar(unsigned char *s, int pct)
{
  if(pct > 100) pct = 100;
  
  // build the string, decide how many blocks
  while(pct)
    {
      int addchar = 0;
      
      if(pct >= BARSIZE)
	{
	  addchar = 123;  // full pct: 4 blocks
	  pct -= BARSIZE;
	}
      else
	{
	  addchar = 127 - (pct*5)/BARSIZE;
	  pct = 0;
	}
      sprintf(s, "%s%c", s, addchar);
    }
}

/******************************
                 Drawer
*******************************/

// the actual drawer is the heart of the overlay
// code. It is split into individual functions,
// each of which draws a different part.

// the offset of percentage bars from the starting text
#define GAP 40

//////////////////////////////////////////////////////////
// draw health

void HU_DrawHealth(int x, int y)
{
  char tempstr[50];
  int fontcolour;
  
  HU_WriteText(HUDCOLOUR "Health", x, y);
  x += GAP;               // leave a gap between name and bar
  
  // decide on the colour first
  fontcolour =
    players[displayplayer].health < health_red ? *FC_RED :
    players[displayplayer].health < health_yellow ? *FC_GOLD :
    players[displayplayer].health <= health_green ? *FC_GREEN :
    *FC_BLUE;
  
  sprintf(tempstr, "%c", fontcolour);

  // now make the actual bar
  HU_TextBar(tempstr, players[displayplayer].health);

  // append the percentage itself
  sprintf(tempstr, "%s %i", tempstr, players[displayplayer].health);

  // write it
  HU_WriteText(tempstr, x, y);
}

////////////////////////////////////////////////////////////
// draw armour. very similar to drawhealth.

void HU_DrawArmor(int x, int y)
{
  char tempstr[50];
  int fontcolour;
  
  // title first
  HU_WriteText(HUDCOLOUR "Armor", x, y);
  x += GAP;              // leave a gap between name and bar
  
  // decide on colour
  fontcolour =
    players[displayplayer].armorpoints < armor_red ? *FC_RED :
    players[displayplayer].armorpoints < armor_yellow ? *FC_GOLD :
    players[displayplayer].armorpoints <= armor_green ? *FC_GREEN :
    *FC_BLUE;
  sprintf(tempstr, "%c", fontcolour);
  
  // make the bar
  HU_TextBar(tempstr, players[displayplayer].armorpoints);
  
  // append the percentage itself
  sprintf(tempstr, "%s %i", tempstr,
	  players[displayplayer].armorpoints);
  
  HU_WriteText(tempstr, x, y);
}

////////////////////////////////////////////////////////
// drawing ammo

void HU_DrawAmmo(int x, int y)
{
  char tempstr[50];
  int fontcolour;
  
  HU_WriteText(HUDCOLOUR "Ammo", x, y);
  x += GAP;
  
  fontcolour = weapcolour(players[displayplayer].readyweapon);
  sprintf(tempstr, "%c", fontcolour);
  
  if(playermaxammo)
    {
      HU_TextBar(tempstr, (100 * playerammo) / playermaxammo);
      sprintf(tempstr, "%s %i/%i", tempstr, playerammo, playermaxammo);
    }
  else    // fist or chainsaw
    sprintf(tempstr, "%sN/A", tempstr);
  
  HU_WriteText(tempstr, x, y);
}

////////////////////////////////////////////
// draw the list of weapons

void HU_DrawWeapons(int x, int y)
{
  char tempstr[50] = "";
  int i;
  int fontcolour;
  
  HU_WriteText(HUDCOLOUR "Weapons", x, y);    // draw then leave a gap
  x += GAP;
  
  for(i=0; i<NUMWEAPONS; i++)
    {
      if(players[displayplayer].weaponowned[i])
	{
	  // got it
	  fontcolour = weapcolour(i);
	  sprintf(tempstr, "%s%c%i ", tempstr,
		  fontcolour, i+1);
	}
    }

  HU_WriteText(tempstr, x, y);    // draw it
}

////////////////////////////////
// draw the keys

extern patch_t *keys[NUMCARDS+3];

void HU_DrawKeys(int x, int y)
{
  int i;
  
  HU_WriteText(HUDCOLOUR "Keys", x, y);    // draw then leave a gap
  x += GAP;
  
  for(i=0; i<NUMCARDS; i++)
    {
      if(players[displayplayer].cards[i])
	{
	  // got that key
	  V_DrawPatch(x, y, 0, keys[i]);
	  x += 11;
	}
    }
}

//////////////////////////////////////
// draw the frags

void HU_DrawFrag(int x, int y)
{
  char tempstr[20];
  
  HU_WriteText(HUDCOLOUR "Frags", x, y);    // draw then leave a gap
  x += GAP;
  
  sprintf(tempstr, HUDCOLOUR "%i", players[displayplayer].totalfrags);
  HU_WriteText(tempstr, x, y);        
}

///////////////////////////////////////
        // draw the status (number of kills etc)
void HU_DrawStatus(int x, int y)
{
  char tempstr[50];
  
  HU_WriteText(HUDCOLOUR "Status", x, y); // draw, leave a gap
  x += GAP;
  
  
  sprintf(tempstr,
	  FC_RED "K" FC_GREEN " %i/%i "
	  FC_RED "I" FC_GREEN " %i/%i "
	  FC_RED "S" FC_GREEN " %i/%i ",
	  players[displayplayer].killcount, totalkills,
	  players[displayplayer].itemcount, totalitems,
	  players[displayplayer].secretcount, totalsecret
	  );
  
  HU_WriteText(tempstr, x, y);
}


overlay_t overlay[NUMOVERLAY];

        // toggle the overlay style
void HU_OverlayStyle()
{
  hud_enabled = true;
  hud_overlaystyle = (hud_overlaystyle+1) % 4;
}

void HU_ToggleHUD()
{
  hud_enabled = !hud_enabled;
}

void HU_OverlaySetup()
{
  int i;
  
  // setup the drawers
  overlay[ol_health].drawer = HU_DrawHealth;
  overlay[ol_ammo].drawer = HU_DrawAmmo;
  overlay[ol_weap].drawer = HU_DrawWeapons;
  overlay[ol_armor].drawer = HU_DrawArmor;
  overlay[ol_key].drawer = HU_DrawKeys;
  overlay[ol_frag].drawer = HU_DrawFrag;
  overlay[ol_status].drawer = HU_DrawStatus;

  //////// now decide where to put all the widgets //////////

  for(i=0; i<NUMOVERLAY; i++)
    overlay[i].x = 1;       // turn em all on
  
  if(hud_hidestatus) overlay[ol_status].x = -1;     // turn off status
  
  if(deathmatch)
    overlay[ol_key].x = -1;         // turn off keys now
  else
    overlay[ol_frag].x = -1;        // turn off frag

  // now build according to style
  
  if(hud_overlaystyle == 0)        // 0 means all turned off
    {
      for(i=0; i<NUMOVERLAY; i++)
	{
	  setol(i, -1, -1);       // turn it off
	}
    }
  else if(hud_overlaystyle == 1)
    {                       // 'bottom left' style
      int y = SCREENHEIGHT-8;
      
      for(i=NUMOVERLAY-1; i >= 0; i--)
	{
	  if(overlay[i].x != -1)
	    {
	      setol(i, 0, y);
	      y -= 8;
	    }
	}
    }
  else if(hud_overlaystyle == 2)  // all at the bottom
    {
      int x, y;
      
      x = 0; y = 192;
      for(i=0; i<NUMOVERLAY; i++)
	{
	  if(overlay[i].x != -1)
	    {
	      setol(i, x, y);
	      x += 160;
	      if(x >= 300)
		{
		  x = 0; y -=8;
		}
	    }
	}
    }
  else if(hud_overlaystyle == 3)
    {
      // similar to boom 'distributed' style
      setol(ol_health, 182, 0);
      setol(ol_armor, 182, 8);
      setol(ol_weap, 182, 184);
      setol(ol_ammo, 182, 192);
      if(deathmatch)  // if dm, put frags in place of keys
        setol(ol_frag, 0, 192)
      else
        setol(ol_key, 0, 192)
      if(!hud_hidestatus)
	setol(ol_status, 0, 184);
    }
}

//     heart of the overlay really.
//     draw the overlay, deciding which bits to draw and where

void HU_OverlayDraw()
{
  int i;
  
  if(viewheight != (SCREENHEIGHT<<hires)) return;  // fullscreen only
  if(automapactive) return;
  if(!hud_enabled) return;
  
  HU_OverlaySetup();
  
  for(i=0; i<NUMOVERLAY; i++)
    {
      if(overlay[i].x != -1)
	overlay[i].drawer(overlay[i].x, overlay[i].y);
    }
}

char *str_style[] =
{
  "off",
  "boom style",
  "flat",
  "distributed",
};

VARIABLE_INT(hud_overlaystyle,  NULL,   0, 3,    str_style);
CONSOLE_VARIABLE(hu_overlay, hud_overlaystyle, 0) {}

VARIABLE_BOOLEAN(hud_hidestatus, NULL, yesno);
CONSOLE_VARIABLE(hu_hidesecrets, hud_hidestatus, 0) {}

void HU_OverAddCommands()
{
  C_AddCommand(hu_overlay);
  C_AddCommand(hu_hidesecrets);
}
