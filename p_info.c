// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Level info.
//
// Under smmu, level info is stored in the level marker: ie. "mapxx"
// or "exmx" lump. This contains new info such as: the level name, music
// lump to be played, par time etc.
//
// By Simon Howard
//
//-----------------------------------------------------------------------------

/* includes ************************/

#include <stdio.h>
#include <stdlib.h>

#include "doomstat.h"
#include "doomdef.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "w_wad.h"
#include "p_setup.h"
#include "p_info.h"
#include "p_mobj.h"
#include "t_script.h"
#include "z_zone.h"

char *info_interpic;
char *info_levelname;
int info_partime;
char *info_music;
char *info_skyname;
char *info_creator="unknown";
char *info_levelpic;
char *info_nextlevel;
char *info_intertext;
char *info_backdrop;
char *info_weapons;
int info_scripts;       // has the current level got scripts?

void P_LowerCase(char *line);
void P_StripSpaces(char *line);
static void P_RemoveEqualses(char *line);
static void P_RemoveComments(char *line);

void P_ParseInfoCmd(char *line);
void P_ParseLevelVar(char *cmd);
void P_ParseInfoCmd(char *line);
void P_ParseScriptLine(char *line);
void P_ClearLevelVars();
void P_ParseInterText(char *line);
void P_InitWeapons();

enum
{
  RT_LEVELINFO,
  RT_SCRIPT,
  RT_OTHER,
  RT_INTERTEXT
} readtype;

void P_LoadLevelInfo(int lumpnum)
{
  char *lump;
  char *rover;
  char *startofline;

  readtype = RT_OTHER;
  P_ClearLevelVars();

  lump = W_CacheLumpNum(lumpnum, PU_STATIC);
  
  rover = startofline = lump;
  
  while(rover < lump+lumpinfo[lumpnum]->size)
    {
      if(*rover == '\n') // end of line
	{
	  *rover = 0;               // make it an end of string (0)
	  P_ParseInfoCmd(startofline);
	  startofline = rover+1; // next line
	  *rover = '\n';            // back to end of line
	}
      rover++;
    }
  Z_Free(lump);
  
  P_InitWeapons();
}

void P_ParseInfoCmd(char *line)
{
  P_CleanLine(line);
  
  if(readtype != RT_SCRIPT)       // not for scripts
    {
      P_StripSpaces(line);
      P_LowerCase(line);
      while(*line == ' ') line++;
      if(!*line) return;
      if((line[0] == '/' && line[1] == '/') ||     // comment
	 line[0] == '#' || line[0] == ';') return;
    }
  
  if(*line == '[')                // a new section seperator
    {
      line++;
      if(!strncmp(line, "level info", 10))
	readtype = RT_LEVELINFO;
      if(!strncmp(line, "scripts", 7))
	{
	  readtype = RT_SCRIPT;
	  info_scripts = true;    // has scripts
	}
      if(!strncmp(line, "intertext", 9))
	readtype = RT_INTERTEXT;
      return;
    }
  
  switch(readtype)
    {
    case RT_LEVELINFO:
      P_ParseLevelVar(line);
      break;
      
    case RT_SCRIPT:
      P_ParseScriptLine(line);
      break;
      
    case RT_INTERTEXT:
      P_ParseInterText(line);
      break;

    case RT_OTHER:
      break;
    }
}

//
//  Level vars: level variables in the [level info] section.
//
//  Takes the form:
//     [variable name] = [value]
//
//  '=' sign is optional: all equals signs are internally turned to spaces
//

enum
{
  IVT_STRING,
  IVT_INT,
  IVT_END
};

typedef struct
{
  int type;
  char *name;
  void *variable;
} levelvar_t;

levelvar_t levelvars[]=
{
  {IVT_STRING,    "levelpic",     &info_levelpic},
  {IVT_STRING,    "levelname",    &info_levelname},
  {IVT_INT,       "partime",      &info_partime},
  {IVT_STRING,    "music",        &info_music},
  {IVT_STRING,    "skyname",      &info_skyname},
  {IVT_STRING,    "creator",      &info_creator},
  {IVT_STRING,    "interpic",     &info_interpic},
  {IVT_STRING,    "nextlevel",    &info_nextlevel},
  {IVT_INT,       "gravity",      &gravity},
  {IVT_STRING,    "inter-backdrop",&info_backdrop},
  {IVT_STRING,    "defaultweapons",&info_weapons},
  {IVT_END,       0,              0}
};

void P_ParseLevelVar(char *cmd)
{
  char varname[50];
  char *equals;
  levelvar_t* current;
  
  if(!*cmd) return;
  
  P_RemoveEqualses(cmd);
  
  // right, first find the variable name
  
  sscanf(cmd, "%s", varname);
  
  // find what it equals
  equals = cmd+strlen(varname);
  while(*equals == ' ') equals++; // cut off the leading spaces
  
  current = levelvars;
  
  while(current->type != IVT_END)
    {
      if(!strcmp(current->name, varname))
	{
	  switch(current->type)
	    {
	    case IVT_STRING:
	      *(char**)current->variable         // +5 for safety
		= Z_Malloc(strlen(equals)+5, PU_LEVEL, NULL);
	      strcpy(*(char**)current->variable, equals);
	      break;
	      
	    case IVT_INT:
	      *(int*)current->variable = atoi(equals);
                     break;
	    }
	}
      current++;
    }
}

// clear all the level variables so that none are left over from a
// previous level

void P_ClearLevelVars()
{
  info_levelname = info_skyname = info_levelpic = "";
  info_music = "";
  info_creator = "unknown";
  info_interpic = "INTERPIC";
  info_partime = -1;
  
  if(gamemode == commercial && isExMy(levelmapname))
    {
      static char nextlevel[10];
      info_nextlevel = nextlevel;
      
      // set the next episode
      strcpy(nextlevel, levelmapname);
      nextlevel[3] ++;
      if(nextlevel[3] > '9')  // next episode
	{
	  nextlevel[3] = '1';
	  nextlevel[1] ++;
	}
      
                info_music = levelmapname;
    }
  else 
    info_nextlevel = "";
  
  info_weapons = "";
  gravity = FRACUNIT;     // default gravity
  info_intertext = info_backdrop = NULL;
  
  T_ClearScripts();
  info_scripts = false;
}

//
// P_ParseScriptLine
//

// Add a line to the levelscript

void P_ParseScriptLine(char *line)
{
  int allocsize;

             // +10 for comfort
  allocsize = strlen(line) + strlen(levelscript.data) + 10;
  
  // realloc the script bigger
  levelscript.data =
    Z_Realloc(levelscript.data, allocsize, PU_LEVEL, 0);
  
  // add the new line to the current data using sprintf (ugh)
  sprintf(levelscript.data, "%s%s\n", levelscript.data, line);
}

void P_CleanLine(char *line)
{
  char *temp;
  
  for(temp=line; *temp; temp++)
    *temp = *temp<32 ? ' ' : *temp;
}

void P_LowerCase(char *line)
{
  char *temp;
  
  for(temp=line; *temp; temp++)
    *temp = tolower(*temp);
}

void P_StripSpaces(char *line)
{
  char *temp;
  
  temp = line+strlen(line)-1;
  
  while(*temp == ' ')
    {
      *temp = 0;
      temp--;
    }
}

static void P_RemoveComments(char *line)
{
  char *temp = line;
  
  while(*temp)
    {
      if(*temp=='/' && *(temp+1)=='/')
	{
	  *temp = 0; return;
	}
      temp++;
    }
}

static void P_RemoveEqualses(char *line)
{
  char *temp;
  
  temp = line;
  
  while(*temp)
    {
      if(*temp == '=')
	{
	  *temp = ' ';
	}
      temp++;
    }
}

        // dumbass fixed-length intertext size
#define INTERTEXTSIZE 1024

void P_ParseInterText(char *line)
{
  while(*line==' ') line++;
  if(!*line) return;
  
  if(!info_intertext)
    {
      info_intertext = Z_Malloc(INTERTEXTSIZE, PU_LEVEL, 0);
      *info_intertext = 0; // first char as the end of the string
    }
  sprintf(info_intertext, "%s%s\n", info_intertext, line);
}

boolean default_weaponowned[NUMWEAPONS];

void P_InitWeapons()
{
  char *s;
  
  memset(default_weaponowned, 0, sizeof(default_weaponowned));
  
  s = info_weapons;
  
  while(*s)
    {
      switch(*s)
	{
	case '3': default_weaponowned[wp_shotgun] = true; break;
	case '4': default_weaponowned[wp_chaingun] = true; break;
	case '5': default_weaponowned[wp_missile] = true; break;
	case '6': default_weaponowned[wp_plasma] = true; break;
	case '7': default_weaponowned[wp_bfg] = true; break;
	case '8': default_weaponowned[wp_supershotgun] = true; break;
	default: break;
	}
      s++;
    }
}

