// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
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

#ifndef __P_CHASE_H__
#define __P_CHASE_H__

#include "p_mobj.h"

typedef struct
{
  long x;
  long y;
  long z;
  angle_t angle;
  int updownangle;
} camera_t;

extern int chasex;
extern int chasey;
extern int chasez;
extern int chaseangle;
extern int chasecam_active;

extern int walkcam_active;

extern camera_t chasecam;
extern camera_t walkcamera;

void P_ChaseSetupFrame();
void P_ChaseTicker();
void P_ChaseStart();
void P_ChaseEnd();
void P_ResetChasecam();
void P_ToggleChasecam();

void P_WalkTicker();
void P_ToggleWalk();
void P_ResetWalkcam();

#endif
