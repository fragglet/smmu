// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
//
// Automap colours.
// Holds console commands (variables) for map colours
//
// By Simon Howard
//
//----------------------------------------------------------------------------

#include "doomdef.h"
#include "c_runcmd.h"

extern int mapcolor_back;    // map background
CONSOLE_INT(mapcolor_back, mapcolor_back, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_grid;    // grid lines color
CONSOLE_INT(mapcolor_grid, mapcolor_grid, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_wall;    // normal 1s wall color
CONSOLE_INT(mapcolor_wall, mapcolor_wall, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_fchg;    // line at floor height change color
CONSOLE_INT(mapcolor_fchg, mapcolor_fchg, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_cchg;    // line at ceiling height change color
CONSOLE_INT(mapcolor_cchg, mapcolor_cchg, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_clsd;    // line at sector with floor=ceiling color
CONSOLE_INT(mapcolor_clsd, mapcolor_clsd, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_rkey;    // red key color
CONSOLE_INT(mapcolor_rkey, mapcolor_rkey, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_bkey;    // blue key color
CONSOLE_INT(mapcolor_bkey, mapcolor_bkey, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_ykey;    // yellow key color
CONSOLE_INT(mapcolor_ykey, mapcolor_ykey, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_rdor;   // red door color  (diff from keys to allow option)
CONSOLE_INT(mapcolor_rdor, mapcolor_rdor, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_bdor;   // blue door color (of enabling one but not other )
CONSOLE_INT(mapcolor_bdor, mapcolor_bdor, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_ydor;    // yellow door color
CONSOLE_INT(mapcolor_ydor, mapcolor_ydor, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_tele;    // teleporter line color
CONSOLE_INT(mapcolor_tele, mapcolor_tele, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_secr;    // secret sector boundary color
CONSOLE_INT(mapcolor_secr, mapcolor_secr, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_exit;    // jff 4/23/98 add exit line color
CONSOLE_INT(mapcolor_exit, mapcolor_exit, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_unsn;    // computer map unseen line color
CONSOLE_INT(mapcolor_unsn, mapcolor_unsn, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_flat;    // line with no floor/ceiling changes
CONSOLE_INT(mapcolor_flat, mapcolor_flat, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_sprt;    // general sprite color
CONSOLE_INT(mapcolor_sprt, mapcolor_sprt, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_hair;    // crosshair color
CONSOLE_INT(mapcolor_hair, mapcolor_hair, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_sngl;    // single player arrow color
CONSOLE_INT(mapcolor_sngl, mapcolor_sngl, NULL, 0, 255, NULL, 0) {}

extern int mapcolor_frnd;    // colors for friends of player
CONSOLE_INT(mapcolor_frnd, mapcolor_frnd, NULL, 0, 255, NULL, 0) {}


void AM_AddColors()
{
  C_AddCommand(mapcolor_back);
  C_AddCommand(mapcolor_wall);
  C_AddCommand(mapcolor_fchg);
  C_AddCommand(mapcolor_grid);
  C_AddCommand(mapcolor_cchg);
  C_AddCommand(mapcolor_clsd);
  C_AddCommand(mapcolor_rkey);
  C_AddCommand(mapcolor_ykey);
  C_AddCommand(mapcolor_bkey);
  C_AddCommand(mapcolor_rdor);
  C_AddCommand(mapcolor_ydor);
  C_AddCommand(mapcolor_bdor);
  C_AddCommand(mapcolor_tele);
  C_AddCommand(mapcolor_secr);
  C_AddCommand(mapcolor_exit);
  C_AddCommand(mapcolor_unsn);
  C_AddCommand(mapcolor_sprt);
  C_AddCommand(mapcolor_hair);
  C_AddCommand(mapcolor_sngl);
  C_AddCommand(mapcolor_frnd);
}
