// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
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

#ifndef __V_MISC_H__
#define __V_MISC_H__

void V_InitMisc();

//---------------------------------------------------------------------------
//
// Font
//

#define V_FONTSTART    '!'     // the first font character
#define V_FONTEND      (0x7f) // jff 2/16/98 '_' the last font characters
// Calculate # of glyphs in font.
#define V_FONTSIZE     (V_FONTEND - V_FONTSTART + 1) 

// font colours
#define FC_BRICK  "\x80"
#define FC_TAN    "\x81"
#define FC_GRAY   "\x82"
#define FC_GREEN  "\x83"
#define FC_BROWN  "\x84"
#define FC_GOLD   "\x85"
#define FC_RED    "\x86"
#define FC_BLUE   "\x87"
#define FC_ORANGE "\x88"
#define FC_YELLOW "\x89"
#define FC_TRANS  "\x8a"

void V_WriteText(unsigned char *s, int x, int y);
void V_WriteTextColoured(unsigned char *s, int colour, int x, int y);
void V_LoadFont();
int V_StringWidth(unsigned char *s);
int V_StringHeight(unsigned char *s);
boolean V_IsPrint(unsigned char c);

//--------------------------------------------------------------------------
//
// FPS ticker
//

void V_FPSDrawer();
void V_FPSTicker();
extern int v_ticker;

//--------------------------------------------------------------------------
//
// Background 'tile' fill
//

void V_DrawBackground(char* patchname, byte *back_dest);
void V_DrawDistortedBackground(char* patchname, byte *back_dest);

//--------------------------------------------------------------------------
//
// Box drawing
//

void V_DrawBox(int, int, int, int);


//--------------------------------------------------------------------------
//
// 'loading' box
//

void V_DrawLoading();
void V_SetLoading(int total, char *mess);
void V_LoadingIncrease();
void V_LoadingSetTo(int amount);



#endif


//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.2  2000-06-20 21:06:43  fraggle
// V_IsPrint function for portable isprint()
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
