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
//      Fixed point arithemtics, implementation.
//
//-----------------------------------------------------------------------------

#ifndef __M_FIXED__
#define __M_FIXED__

#ifndef __GNUC__
#define __inline__
#define __attribute__(x)
#endif

#include "i_system.h"

//
// Fixed point, 32bit as 16.16.
//

#define FRACBITS 16
#define FRACUNIT (1<<FRACBITS)

typedef int fixed_t;

//
// Absolute Value
//

// killough 5/10/98: In djgpp, use inlined assembly for performance
// killough 9/05/98: better code seems to be gotten from using inlined C

#ifdef I386
#define abs(x) ({fixed_t _t = (x), _s = _t >> (8*sizeof _t-1); (_t^_s)-_s;})
#endif // I386

//
// Fixed Point Multiplication
//

#ifdef I386

// killough 5/10/98: In djgpp, use inlined assembly for performance
// sf: code imported from lxdoom

inline static const fixed_t FixedMul(fixed_t a, fixed_t b)
{
  fixed_t result;
  int dummy;

  asm("  imull %3 ;"
      "  shrdl $16,%1,%0 ;"
      : "=a" (result),          /* eax is always the result */
        "=d" (dummy)		/* cphipps - fix compile problem with gcc-2.95.1
				   edx is clobbered, but it might be an input */
      : "0" (a),                /* eax is also first operand */
        "r" (b)                 /* second operand could be mem or reg before,
				   but gcc compile problems mean i can only us reg */
      : "%cc"                   /* edx and condition codes clobbered */
      );

  return result;
}

#else // I386

__inline__ static fixed_t FixedMul(fixed_t a, fixed_t b)
{
  return (fixed_t)((long long) a*b >> FRACBITS);
}

#endif // I386

//
// Fixed Point Division
//

#ifdef I386

// killough 5/10/98: In djgpp, use inlined assembly for performance
// killough 9/5/98: optimized to reduce the number of branches
// sf: code imported from lxdoom

inline static const fixed_t FixedDiv(fixed_t a, fixed_t b)
{
  if (abs(a) >> 14 < abs(b))
    {
      fixed_t result;
      int dummy;
      asm(" idivl %4 ;"
	  : "=a" (result),
	    "=d" (dummy)  /* cphipps - fix compile problems with gcc 2.95.1
			     edx is clobbered, but also an input */
	  : "0" (a<<16),
	    "1" (a>>16),
	    "r" (b)
	  : "%cc"
	  );
      return result;
    }
  return ((a^b)>>31) ^ INT_MAX;
}

#else // I386

__inline__ static fixed_t FixedDiv(fixed_t a, fixed_t b)
{
  return (abs(a)>>14) >= abs(b) ? ((a^b)>>31) ^ MAXINT :
    (fixed_t)(((long long) a << FRACBITS) / b);
}

#endif // I386

#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.2  2000-06-09 20:51:09  fraggle
// fix i386 asm for v2 djgpp
//
// Revision 1.1.1.1  2000/04/30 19:12:08  fraggle
// initial import
//
//
//----------------------------------------------------------------------------

