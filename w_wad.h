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
//      WAD I/O functions.
//
//-----------------------------------------------------------------------------

#ifndef __W_WAD__
#define __W_WAD__

#include <stdio.h>

//
// TYPES
//

typedef struct
{
  char identification[4];                  // Should be "IWAD" or "PWAD".
  int  numlumps;
  int  infotableofs;
} wadinfo_t;

typedef struct
{
  int  filepos;
  int  size;
  char name[8];
} filelump_t;

//
// WADFILE I/O related stuff.
//

typedef struct
{
  // WARNING: order of some fields important (see info.c).

  char  name[8];
  int   size;
  const void *data;     // killough 1/31/98: points to predefined lump data

  // killough 1/31/98: hash table fields, used for ultra-fast hash table lookup
  int index, next;

  // killough 4/17/98: namespace tags, to prevent conflicts between resources
  enum {
    ns_global=0,
    ns_sprites,
    ns_flats,
    ns_colormaps
  } namespace;

  int handle;
  int position;
  void *cache;  //sf
} lumpinfo_t;

// killough 1/31/98: predefined lumps
extern const size_t num_predefined_lumps;
extern const lumpinfo_t predefined_lumps[];

extern void       **lumpcache;
extern lumpinfo_t **lumpinfo;   //sf: ptr to ptr
extern int        numlumps;

void W_InitMultipleFiles(char *const*filenames);

// sf: add a new wad file after the game has already begun
int W_AddNewFile(char *filename);

// killough 4/17/98: if W_CheckNumForName() called with only
// one argument, pass ns_global as the default namespace

#define W_CheckNumForName(name) (W_CheckNumForName)(name, ns_global)
int     (W_CheckNumForName)(const char* name, int);   // killough 4/17/98
int     W_GetNumForName (const char* name);
int     W_LumpLength (int lump);
void    W_ReadLump (int lump, void *dest);
void*   W_CacheLumpNum (int lump, int tag);
long    W_LumpCheckSum (int lumpnum);

#define W_CacheLumpName(name,tag) W_CacheLumpNum (W_GetNumForName(name),(tag))

void NormalizeSlashes(char *);                    // killough 11/98
char *AddDefaultExtension(char *, const char *);  // killough 1/18/98
void ExtractFileBase(const char *, char *);       // killough
unsigned W_LumpNameHash(const char *s);           // killough 1/31/98
void W_InitLumpHash(void);

void V_BeginRead(void), V_EndRead(void); // killough 10/98

// Function to write all predefined lumps to a PWAD if requested
extern void WritePredefinedLumpWad(const char *filename); // jff 5/6/98

extern int iwadhandle;
#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:09  fraggle
// Initial revision
//
//
//----------------------------------------------------------------------------
