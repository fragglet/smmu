// Emacs style mode select -*- C++ -*-
//------------------------------------------------------------------------
//
// Wad routines, ripped from my old wadptr program
//
// By Simon Howard
//
//-----------------------------------------------------------------------

#ifndef __WADDIR_H_INCLUDED__
#define __WADDIR_H_INCLUDED__

typedef enum
{
  IWAD,
  PWAD,
  NONWAD
} wadtype;

typedef struct
{
  long offset;
  long length;
  char name[8];
} entry_t;

void readwad();
void writewad();
char *convert_string8(entry_t entry);
entry_t *findinfo(char *entrytofind);
void addentry(entry_t *entry);
int entry_exist(char *entrytofind);
void *cachelump(int entrynum);
void copywad(char *newfile);

int islevel(int entry);
int isnum(char n);
int islevelname(char *s);

extern FILE *wadfp;
extern char picentry[8];
extern long numentries, diroffset;
extern entry_t *wadentry;
extern wadtype wad;

#endif
