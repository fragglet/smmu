// Emacs style mode select -*- C++ -*-
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

#ifndef __C_IO_H__
#define __C_IO_H__

//#include "doomstat.h"
#include "d_event.h"
        // for text colours:
#include "v_video.h"

#define INPUTLENGTH 512
#define LINELENGTH 96

void C_Init();
void C_Ticker();
void C_Drawer();
int C_Responder(event_t* ev);
void C_Update();

void C_Printf(unsigned char *s, ...);
void C_WriteText(unsigned char *s, ...);
#define C_Puts C_WriteText     /*** basically the same **/
void C_Seperator();

void C_SetConsole();
void C_Popup();
void C_InstaPopup();

// sf 9/99: made a #define
#define consoleactive (current_height || gamestate==GS_CONSOLE)

extern int c_height;     // the height of the console
extern int c_speed;       // pixels/tic it moves
extern int current_height;
extern int current_target;
#define c_moving (current_height != current_target)
extern boolean c_showprompt;

#endif

//--------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:08  fraggle
// Initial revision
//
//
//--------------------------------------------------------------------------
