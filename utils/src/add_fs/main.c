// Emacs style mode select -*- C++ -*-
//-------------------------------------------------------------------------
//
// utility tool for SMMU
//
// Load a Level file (FraggleScript) into a wad.
//
//------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "waddir.h"

static void add_fs();

char *wadfile_name;
int level_entry;                // entry number of level header

char *fsfile_name;
char *fsfile_data;
int fsfile_len;

void main(int argc, char *argv[])
{
  // display notice
  printf
    (
     "-------------------------------------\n"
     "add_fs\n"
     "by Simon Howard 'Fraggle'\n"
     "http://fraggle.tsx.org/\n"
     "distributed under GNU GPL\n"
     "\n"
     );
  
  if(argc < 3)
    {
      printf("usage: add_fs wadfile levelfile\n");
      exit(-1);
    }
  
  wadfile_name = argv[1];
  fsfile_name = argv[2];
  
  add_fs();
}

        // stolen from boom: w_wad.c
static int FileLength(int handle)
{
  struct stat fileinfo;
  if (fstat(handle, &fileinfo) == -1)
    errorexit("Error fstating");
  return fileinfo.st_size;
}

#ifndef O_BINARY        /** for non-dos os's **/
 #define O_BINARY
#endif

// read the FraggleScript file into buffer

static void read_fsfile()
{
  int handle;
  
  // open file
  handle = open(fsfile_name, O_BINARY | O_RDONLY);
  
  // find file length
  fsfile_len = FileLength(handle);
  fsfile_data = malloc(fsfile_len);
  
  // read file
  read(handle, fsfile_data, fsfile_len);
  
  close(handle);  // close
  
  printf("%s loaded: %i bytes\n", fsfile_name, fsfile_len);
}

static void open_wadfile()
{
  // load and read wad
  wadfp = fopen(wadfile_name, "rb+");
  readwad();
  
  printf("%s: %i entries at %i\n", wadfile_name, numentries, diroffset);
  
  // "THINGS" always follows the level header
  level_entry = entry_exist("THINGS");
  if(level_entry == -1) errorexit("level not found!\n");
  else
    level_entry--;    // actually the previous entry
  
  printf("level: %s\n", convert_string8(wadentry[level_entry]));
}

        // actually add the file in
static void add_fs()
{
  read_fsfile();
  open_wadfile();
  
  // We need to go to the wad directory and overwrite this with
  // the FraggleScript file. The new wad directory can then
  // be tacked onto the end of the data.
  
  fsetpos(wadfp, &diroffset);
  
  // write the data into the wad
  
  fwrite(fsfile_data, fsfile_len, sizeof(char), wadfp);
  
  // update wad entry
  
  wadentry[level_entry].offset = diroffset;
  wadentry[level_entry].length = fsfile_len;

  // update the directory offset to its new position at the
  // end of the added data
  
  diroffset += fsfile_len;
  
  // re-write the wad directory
  
  writewad();
  
  // clean up
  
  fclose(wadfp);
  free(fsfile_data);
  
  // probably a good idea to run a wad cleaner after this.
  
  printf("%s added to %s ok\n", fsfile_name, wadfile_name);
}
