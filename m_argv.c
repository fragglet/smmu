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
//
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id$";

#include <string.h>

int    myargc;
char **myargv;

//
// M_CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1)
// or 0 if not present
//

int M_CheckParm(const char *check)
{
  int i;
  for (i=1; i<myargc; i++)
    if (!strcasecmp(check, myargv[i]))
      return i;
  return 0;
}

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-07-29 13:20:39  fraggle
// Initial revision
//
// Revision 1.5  1998/05/03  22:51:40  killough
// beautification
//
// Revision 1.4  1998/05/01  14:26:14  killough
// beautification
//
// Revision 1.3  1998/05/01  14:23:29  killough
// beautification
//
// Revision 1.2  1998/01/26  19:23:40  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:58  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
