// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
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
//--------------------------------------------------------------------------
//
// Menu file selector
//
// eg. For selecting a wad to load or demo to play
//
// By Simon Howard
//
//---------------------------------------------------------------------------

#include <stdio.h>
#include <dirent.h>
#include "c_io.h"
#include "mn_engin.h"
#include "z_zone.h"

////////////////////////////////////////////////////////////////////////////
//
// Read Directory
//
// Read all the files in a particular directory that fit a particular
// wildcard
//

// see if a filename matches a particular wildcard

static boolean filecmp(char *filename, char *wildcard)
{
  char *filename_main, *wildcard_main;      // filename
  char *filename_ext, *wildcard_ext;        // extension
  int i;
  
  filename_main = Z_Strdup(filename, PU_CACHE, 0);
  wildcard_main = Z_Strdup(wildcard, PU_CACHE, 0);
  
  // find seperator
  filename_ext = strchr(filename_main,'.');
  if(!filename_ext) filename_ext = ""; // no extension
  else
    {
      // break into 2 strings
      // replace . with \0
      *filename_ext = '\0';
      filename_ext++;
    }

  // do the same with the wildcard
  
  wildcard_ext = strchr(wildcard_main, '.');
  if(!wildcard_ext) wildcard_ext = "";
  else
    {
      *wildcard_ext=0;
      wildcard_ext++;
    }

  // compare main part of filename with wildcard
  
  for(i=0; ; i++)    // compare the filenames
    {
      if(wildcard_main[i]=='?') continue;
      if(wildcard_main[i]=='*') break;
      if(wildcard_main[i]!=filename_main[i]) return false;
      if(wildcard_main[i]=='\0') break; // end of string
    }
  
  for(i=0; ;i++)    // compare the extensions
    {
      if(wildcard_ext[i]=='?') continue;
      if(wildcard_ext[i]=='*') break;
      if(wildcard_ext[i]!=filename_ext[i]) return false;
      if(wildcard_ext[i]=='\0') break; // end of string
    }

  // compare ok
  return true;
}

static int num_files;
static char **filelist;
static char *filedir;      // directory the files are in

static void AddFile(char *filename)
{
  static int allocedsize = 0;

  if(num_files >= allocedsize)
    {
      // realloc bigger: limitless
      if(!allocedsize)
	{
	  allocedsize = 128;
	  filelist = Z_Malloc(sizeof(char *) * allocedsize, PU_STATIC, 0);
	}
      else
	{
	  allocedsize *= 2;
	  filelist = Z_Realloc(filelist, sizeof(char *) * allocedsize,
			       PU_STATIC, 0);
	}
    }

  filelist[num_files++] = Z_Strdup(filename, PU_STATIC, 0);
}

static void ClearDirectory()
{
  int i;

  // clear all alloced files

  for(i=0; i<num_files; i++)
    Z_Free(filelist[i]);
    
  num_files = 0;  
}

// sort files
// a form of quicksort

static void SortFiles(int start, int num)
{
  char **lesslist, **greaterlist;
  int num_less, num_greater;
  char *pivot;
  int i;

  if(num < 2) return;  // nothing to sort

  // we use the filelist to store the lesslist as it is faster
  // -- no need to malloc and free mem
  
  lesslist = &filelist[start]; num_less = 0;
  greaterlist = malloc(sizeof(char *) * num); num_greater = 0;

  // use first in list as pivot
  pivot = filelist[start];

  // sort into 2 lists: ignore pivot
  
  for(i=start+1; i<start+num; i++)
    {
      // strcmp is a fast way to compare the filenames
      if(strcasecmp(filelist[i], pivot) < 0)
	lesslist[num_less++] = filelist[i];
      else
	greaterlist[num_greater++] = filelist[i];
    }

  // include pivot
  filelist[start+num_less] = pivot;
  
  // copy greaterlist back into filelist after pivot

  memcpy(&filelist[start+num_less+1],
	 greaterlist,
	 sizeof(char*) * num_greater);
  free(greaterlist);

  // sort individual lists: recursive sort

  SortFiles(start, num_less);
  SortFiles(start+num_less+1, num_greater);
}

static void ReadDirectory(char *read_dir, char *read_wildcard)
{
  DIR *directory;
  struct dirent *direntry;

  // clear directory
  ClearDirectory();
  
  // open directory and read filenames  
  filedir = read_dir;
  directory = opendir(read_dir);

  if(!directory) return;
  
  while (1)
    {
      direntry = readdir(directory);
      if(!direntry) break;
      if(filecmp(direntry->d_name, read_wildcard))
	AddFile(direntry->d_name);       // add file to list
    }
  closedir(directory);

  SortFiles(0, num_files);
}

///////////////////////////////////////////////////////////////////////////
//
// File Selector
//
// Used as a 'browse' function when we are selecting some kind of file
// to load: eg. lmps, wads
//

#define PAGESIZE 16 /* number of filenames to draw in selector window */
#define FILEWIN_X 60
#define FILEWIN_Y 40

void MN_FileDrawer();
boolean MN_FileResponder(event_t *ev);

// file selector is handled using a menu widget

menuwidget_t file_selector = {MN_FileDrawer, MN_FileResponder};
static int selected_item;
static char *variable_name;
static char *help_description;

void MN_FileDrawer()
{
  int i, item;

  // draw background first

  V_DrawBackground(background_flat, screens[0]);

  // draw help description

  if(help_description)
    MN_WriteTextColoured
      (
       help_description,
       CR_GOLD,
       FILEWIN_X,
       20
       );
  
  // draw filenames

  for(i=0, item=selected_item-6; i<PAGESIZE; i++, item++)
    {
      if(item < 0 || item >= num_files) continue;
      
      MN_WriteTextColoured
	(
	 filelist[item],
	 item==selected_item ? CR_GRAY : CR_RED,
	 FILEWIN_X,
	 FILEWIN_Y + i*8
	 );
    }
}

boolean MN_FileResponder(event_t *ev)
{
  if(ev->type != ev_keydown) return false;    // don't care

  if(ev->data1 == KEYD_UPARROW)
    {
      if(selected_item > 0) selected_item--;
      return true;
    }

  if(ev->data1 == KEYD_DOWNARROW)
    {
      if(selected_item < (num_files-1)) selected_item++;
      return true;
    }

  if(ev->data1 == KEYD_PAGEUP)
    {
      selected_item -= PAGESIZE;
      if(selected_item < 0) selected_item = 0;
      return true;
    }

  if(ev->data1 == KEYD_PAGEDOWN)
    {
      selected_item += PAGESIZE;
      if(selected_item >= num_files) selected_item = num_files-1;
      return true;
    }
  
  if(ev->data1 == KEYD_ESCAPE || ev->data1 == KEYD_BACKSPACE)
    current_menuwidget = NULL;

  if(ev->data1 == KEYD_ENTER)
    {
      // set variable to new value
      if(variable_name)
	{
	  char tempstr[128];
	  sprintf(tempstr, "%s %s/%s", variable_name,
		  filedir, filelist[selected_item]);
	  cmdtype = c_menu;
	  C_RunTextCmd(tempstr);
	}
      current_menuwidget = NULL;       // cancel widget
      return true;
    }
  
  return true;
}

char *wad_directory;   // directory where user keeps wads

VARIABLE_STRING(wad_directory, NULL,        30);
CONSOLE_VARIABLE(wad_directory, wad_directory, 0)
{
  char *a;
  for(a = wad_directory; *a; a++)
    if(*a == '\\') *a = '/';
}

CONSOLE_COMMAND(mn_selectwad, 0)
{
  ReadDirectory(wad_directory, "*.wad");
  if(num_files < 1)
    {
      MN_ErrorMsg("no files found");
      return;
    }
  selected_item = 0;
  current_menuwidget = &file_selector;
  help_description = "select wad file:";
  variable_name = "mn_wadname";
}

CONSOLE_COMMAND(mn_selectlmp, 0)
{
  ReadDirectory(".", "*.lmp");
  selected_item = 0;
  current_menuwidget = &file_selector;
  help_description = "select demo:";
  variable_name = "mn_demoname";
}

//////////////////////////////////////////////////////////////////////////
//
// Misc stuff
//
// The Filename reading provides a useful opportunity for some handy
// console commands.
//
  
// 'dir' command is useful :)

CONSOLE_COMMAND(dir, 0)
{
  int i;
  char *wildcard;
  
  if(c_argc)
    wildcard = c_argv[0];
  else
    wildcard = "*.*";

  ReadDirectory(".", wildcard);

  for(i=0; i<num_files; i++)
    {
      C_Puts(filelist[i]);
    }
}

void MN_File_AddCommands()
{
  C_AddCommand(dir);
  C_AddCommand(mn_selectwad);
  C_AddCommand(mn_selectlmp);
  C_AddCommand(wad_directory);
}

//-------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:08  fraggle
// Initial revision
//
//
//-------------------------------------------------------------------------
