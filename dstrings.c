// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// DESCRIPTION:
//	Globally defined strings.
// 
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id$";


#ifdef __GNUG__
#pragma implementation "dstrings.h"
#endif
#include "dstrings.h"

// sf 29/3/2000 : dont say dos if this is linux

char* endmsg[/*NUM_QUITMESSAGES+1*/]=
{
  // DOOM1
  QUITMSG,
  "please don't leave, there's more\ndemons to toast!",
  "let's beat it -- this is turning\ninto a bloodbath!",
#ifdef DJGPP
  "i wouldn't leave if i were you.\ndos is much worse.",
  "you're trying to say you like dos\nbetter than me, right?",
#else
  "i wouldn't leave if i were you.\nthe shell is much worse.",
  "you're trying to say you like your shell\nbetter than me, right?",
#endif
  "don't leave yet -- there's a\ndemon around that corner!",
  "ya know, next time you come in here\ni'm gonna toast ya.",
  "go ahead and leave. see if i care.",

  // QuitDOOM II messages
  "you want to quit?\nthen, thou hast lost an eighth!",
#ifdef DJGPP
  "don't go now, there's a \ndimensional shambler waiting\nat the dos prompt!",
#else
  "don't go now, there's a \ndimensional shambler waiting\nat the shell prompt!",
#endif
  "get outta here and go back\nto your boring programs.",
  "if i were your boss, i'd \n deathmatch ya in a minute!",
  "look, bud. you leave now\nand you forfeit your body count!",
  "just leave. when you come\nback, i'll be waiting with a bat.",
  "you're lucky i don't smack\nyou for thinking about leaving.",

  // Internal debug. Different style, too.
  "What? How can you see this message?\nGo away."
};

//---------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:08  fraggle
// Initial revision
//
//
//---------------------------------------------------------------------------
  


