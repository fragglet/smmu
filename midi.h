// Emacs style mode select -*- C++ -*-
//-------------------------------------------------------------------------
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//     Allegro MIDI structure: so we can use mmus2mid without
//     allegro
//
//-----------------------------------------------------------------------------

#ifndef __MIDI_H__
#define __MIDI_H__

// in djgpp version this is declared in allegro.h

#ifndef DJGPP

#define MIDI_TRACKS           32       /* able to handle this many */

typedef struct MIDI                    /* a midi file */
{
   int divisions;                      /* number of ticks per quarter note */
   struct {
      unsigned char *data;             /* MIDI message stream */
      int len;                         /* length of the track data */
   } track[MIDI_TRACKS]; 
} MIDI;

#endif /* #ifndef DJGPP */

#endif
