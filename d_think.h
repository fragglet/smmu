// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
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
// DESCRIPTION:
//  MapObj data. Map Objects or mobjs are actors, entities,
//  thinker, take-your-pick... anything that moves, acts, or
//  suffers state changes of more or less violent nature.
//
//-----------------------------------------------------------------------------

#ifndef __D_THINK__
#define __D_THINK__

// killough 11/98: convert back to C instead of C++
typedef  void (*actionf_t)();

// Historically, "think_t" is yet another function 
// pointer to a routine to handle an actor.
typedef actionf_t think_t;

// Doubly linked list of actors.
typedef struct thinker_s
{
  struct thinker_s *prev, *next;
  think_t function;
  
  // killough 8/29/98: we maintain thinkers in several equivalence classes,
  // according to various criteria, so as to allow quicker searches.

  struct thinker_s *cnext, *cprev; // Next, previous thinkers in same class

  // killough 11/98: count of how many other objects reference
  // this one using pointers. Used for garbage collection.
  unsigned references;
} thinker_t;

#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-07-29 13:20:41  fraggle
// Initial revision
//
// Revision 1.3  1998/05/04  21:34:20  thldrmn
// commenting and reformatting
//
// Revision 1.2  1998/01/26  19:26:34  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:08  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
