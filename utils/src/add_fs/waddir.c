// Emacs style mode select -*- C++ -*-
//------------------------------------------------------------------------
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
// Wad routines, ripped from my old wadptr program
// By Simon Howard
//
//-----------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "waddir.h"

enum { false, true };

//////////////////////////////////////////////////////////////////
//
// Portable read/write
//

unsigned short read_short(FILE *fstream)
{
  unsigned char c[2];

  fread(c, 2, 1, fstream);

  return c[0] + (c[1] << 8);
}

unsigned long read_long(FILE *fstream)
{
  unsigned char c[4];
  fread(c, sizeof(c), 1, fstream);

  return c[0] + (c[1] << 8) + (c[2] << 16) + (c[3] << 24);
}
  
void write_short(unsigned short s, FILE *fstream)
{
  unsigned char c;

  c = (s) & 255;       fwrite(&c, sizeof(c), 1, fstream);
  c = (s >>= 8) & 255; fwrite(&c, sizeof(c), 1, fstream);
}

void write_long(unsigned long l, FILE *fstream)
{
  unsigned char c;
  
  c = (l) & 255;       fwrite(&c, sizeof(c), 1, fstream);
  c = (l >>= 8) & 255; fwrite(&c, sizeof(c), 1, fstream);
  c = (l >>= 8) & 255; fwrite(&c, sizeof(c), 1, fstream);
  c = (l >>= 8) & 255; fwrite(&c, sizeof(c), 1, fstream);
}

FILE *wadfp;
long numentries, diroffset;
entry_t *wadentry = NULL;          // sf: removed MAXENTRIES
wadtype wad;

// exit with error

void errorexit(char *s, ...)
{
  va_list args;
  va_start(args, s);
  
  vfprintf(stderr, s, args);
  
  exit(0xff);
}

///////////////////////////////////////////////////////////////////////////
// wad directory read/write

// read wad

void readwad()
{
  char str[4];
  int i;
  
  // read wad header
  
  rewind(wadfp);

  // find if iwad/pwad
  
  fread(str, 4, 1, wadfp);  
  
  if(!strncmp(str, "PWAD", 4)) wad=PWAD;
  else if(!strncmp(str, "IWAD", 4)) wad=IWAD;
  else
    errorexit("readwad: File not IWAD or PWAD!\n");

  numentries = read_long(wadfp);
  diroffset = read_long(wadfp);
  
  fsetpos(wadfp, &diroffset);
  
  // removed MAXENTRIES limit

  if(wadentry)
    free(wadentry);

  wadentry = malloc(sizeof(*wadentry) * numentries);
  
  // read the wad entries
  // now portable code

  for(i=0; i<numentries; i++)
    {
      wadentry[i].offset = read_long(wadfp);
      wadentry[i].length = read_long(wadfp);
      fread(wadentry[i].name, 8, sizeof(char), wadfp);
    }
}

// write wad header and directory

void writewad()
{
  int i;
  
  rewind(wadfp);
  
  fwrite(wad == IWAD ? "IWAD" : "PWAD", 1, 4,wadfp);
  write_long(numentries, wadfp);
  write_long(diroffset, wadfp);
  
  // write all the entries
  // now portable code
  
  fsetpos(wadfp, &diroffset);

  for(i=0; i<numentries; i++)
    {
      write_long(wadentry[i].offset, wadfp);
      write_long(wadentry[i].length, wadfp);
      fwrite(wadentry[i].name, sizeof(char), 8, wadfp);
    }
}

// get the name of an entry

char *convert_string8(entry_t entry)
{
  static int tempnum=1;
  char temp[9];

  strncpy(temp, entry.name, 8);
  temp[8] = '\0';

  return strdup(temp);
}

// find if an entry exists
// arrrgghh! slow slow slow

int entry_exist(char *entrytofind)
{
  int count;

  for(count=0;count<numentries;count++)
    {
      if(!strncmp(wadentry[count].name, entrytofind, 8))
	return count;
    }

  return -1;
}

// find an entry
// arrggh! again

entry_t *findinfo(char *entrytofind)
{
  int count;
  char buffer[10];
  
  for(count=0;count<numentries;count++)
    {
      if(!strncmp(wadentry[count].name,entrytofind,8 ))
	return &wadentry[count];
    }
  
  return NULL;
}

// add an entry to wad list

void addentry(entry_t *entry)
{
  char buffer[10];
  long temp;
  
  // realloc bigger and add entry to end
  wadentry = realloc(wadentry, (numentries+1) * sizeof(*wadentry));

  memcpy(&wadentry[numentries++], &entry, sizeof(*entry));
  writewad();
}

// load a lump to memory
void *cachelump(int entrynum)
{
  char *working;
  
  working = malloc(wadentry[entrynum].length);
  if(!working)
    errorexit("cachelump: Couldn't malloc %i bytes\n",
	      wadentry[entrynum].length);
  
  fsetpos(wadfp, &wadentry[entrynum].offset);
  fread(working, wadentry[entrynum].length, 1, wadfp);
  
  return working;
}

// various wad related functions

int islevel(int entry)
{
  if(entry >= numentries) return false;
  
  // 9/9/99: generalised support: if the next entry is a
  // things resource then its a level
  return !strncmp(wadentry[entry+1].name, "THINGS", 8);
}

int islevelentry(char *s)
{
  if(!strncmp(s, "LINEDEFS", 8)) return true;
  if(!strncmp(s, "SIDEDEFS", 8)) return true;
  if(!strncmp(s, "SECTORS", 8))  return true;
  if(!strncmp(s, "VERTEXES", 8)) return true;
  if(!strncmp(s, "REJECT", 8))   return true;
  if(!strncmp(s, "BLOCKMAP", 8)) return true;
  if(!strncmp(s, "NODES", 8))    return true;
  if(!strncmp(s, "THINGS", 8))   return true;
  if(!strncmp(s, "SEGS", 8))     return true;
  if(!strncmp(s, "SSECTORS", 8)) return true;
  
  if(!strncmp(s, "BEHAVIOR", 8)) return true; // hexen "behavior" lump
  return false;
}

// find total size of a level

int findlevelsize(char *s)
{
  int entrynum, count, sizecount=0;
  
  entrynum=entry_exist(s);
  
  for(count=entrynum+1;count<entrynum+11;count++)
    sizecount+=wadentry[count].length;
  
  return sizecount;
}

//------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:12  fraggle
// Initial revision
//
//
//------------------------------------------------------------------------
