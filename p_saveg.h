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
//      Savegame I/O, archiving, persistence.
//
//-----------------------------------------------------------------------------

#ifndef __P_SAVEG__
#define __P_SAVEG__

// Persistent storage/archiving.
// These are the load / save game routines.
void P_ArchivePlayers(void);
void P_UnArchivePlayers(void);
void P_ArchiveWorld(void);
void P_UnArchiveWorld(void);
void P_ArchiveThinkers(void);
void P_UnArchiveThinkers(void);
void P_ArchiveSpecials(void);
void P_UnArchiveSpecials(void);
void P_ArchiveScripts(void);
void P_UnArchiveScripts(void);

// 1/18/98 killough: add RNG info to savegame
void P_ArchiveRNG(void);
void P_UnArchiveRNG(void);

// 2/21/98 killough: add automap info to savegame
void P_ArchiveMap(void);
void P_UnArchiveMap(void);

void P_FreeObjTable();
void P_NumberObjects();
void P_DeNumberObjects();

extern byte *save_p;
void CheckSaveGame(size_t);              // killough

#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:09  fraggle
// Initial revision
//
//
//----------------------------------------------------------------------------
