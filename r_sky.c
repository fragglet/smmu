// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_sky.c,v 1.6 1998/05/03 23:01:06 killough Exp $
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
//  Sky rendering. The DOOM sky is a texture map like any
//  wall, wrapping around. A 1024 columns equal 360 degrees.
//  The default sky map is 256 columns and repeats 4 times
//  on a 320 screen?
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: r_sky.c,v 1.6 1998/05/03 23:01:06 killough Exp $";

#include "doomstat.h"
#include "r_sky.h"
#include "r_data.h"
#include "p_info.h"

//
// sky mapping
//

int skyflatnum;
int skytexture;
int skytexturemid;
int stretchsky=1;

//
// R_InitSkyMap
// Called whenever the view size changes.
//
void R_InitSkyMap (void)
{
  skytexturemid = 100*FRACUNIT;
}

// called when the level starts to load the appropriate sky

void R_StartSky()
{
  char *texturename=0;
  // Set the sky map.
  // First thing, we have a dummy sky texture name,
  //  a flat. The data is in the WAD only because
  //  we look for an actual index, instead of simply
  //  setting one.

  skyflatnum = R_FlatNumForName ( SKYFLATNAME );

  // DOOM determines the sky texture to be used
  // depending on the current episode, and the game version.
  if (gamemode == commercial)
    // || gamemode == pack_tnt   //jff 3/27/98 sorry guys pack_tnt,pack_plut
    // || gamemode == pack_plut) //aren't gamemodes, this was matching retail
    {
        texturename="SKY3";
        if(gamemap<13) texturename="SKY1";
        else
          if(gamemap<21) texturename="SKY2";
    }
  else //jff 3/27/98 and lets not forget about DOOM and Ultimate DOOM huh?
    switch (gameepisode)
      {
      case 1:
        texturename="SKY1";
	break;
      case 2:
#ifdef BETA
	// killough 10/98: beta version had different sky orderings
        texturename=beta_emulation ? "SKY1" : "SKY2";
#else
        texturename="SKY2";
#endif
	break;
      case 3:
        texturename="SKY3";
	break;
      case 4: // Special Edition sky
        texturename="SKY4";
	break;
      }//jff 3/27/98 end sky setting fix

      if(*info_skyname)
      {
              texturename=info_skyname;
      }

      skytexture = R_TextureNumForName (texturename);

}

//----------------------------------------------------------------------------
//
// $Log: r_sky.c,v $
// Revision 1.6  1998/05/03  23:01:06  killough
// beautification
//
// Revision 1.5  1998/05/01  14:14:24  killough
// beautification
//
// Revision 1.4  1998/02/05  12:14:31  phares
// removed dummy comment
//
// Revision 1.3  1998/01/26  19:24:49  phares
// First rev with no ^Ms
//
// Revision 1.2  1998/01/19  16:17:59  rand
// Added dummy line to be removed later.
//
// Revision 1.1.1.1  1998/01/19  14:03:07  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
