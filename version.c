// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: version.c,v 1.2 1998/05/03 22:59:31 killough Exp $
//
//-----------------------------------------------------------------------------

static const char rcsid[] = "$Id: version.c,v 1.2 1998/05/03 22:59:31 killough Exp $";

#include "version.h"

int VERSION = 310;        // sf: made int from define 
const char version_date[] = __DATE__;
const char version_name[] = "fs-beta";  // sf : version names
                                        // at the suggestion of mystican

//----------------------------------------------------------------------------
//
// $Log: version.c,v $
// Revision 1.2  1998/05/03  22:59:31  killough
// beautification
//
// Revision 1.1  1998/02/02  13:21:58  killough
// version information files
//
//----------------------------------------------------------------------------
