// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// By Popular demand :)
// Hubs.
//
// Hubs are where there is more than one level, and links between them:
// you can freely travel between all the levels in a hub and when you
// return, the level should be exactly as it previously was.
// As in Quake2/Half life/Hexen etc.
//
// By Simon Howard
//
//----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include "c_io.h"
#include "g_game.h"
#include "p_setup.h"
#include "z_zone.h"

#define MAXHUBLEVELS 128

typedef struct hublevel_s hublevel_t;
struct hublevel_s
{
  char levelname[8];
  char *tmpfile;        // temporary file holding the saved level
};

extern char *gamemapname;

hublevel_t hub_levels[MAXHUBLEVELS];
int num_hub_levels;

// sf: my own tmpnam (djgpp one doesn't work as i want it)

char *tmpnam2()
{
  static int tmpfilenum = 0;
  char *new_tmpfilename;

  new_tmpfilename = malloc(10);

  sprintf(new_tmpfilename, "%i.tmp", tmpfilenum++);

  return new_tmpfilename;  
}

void P_ClearHubs()
{
  int i;

  for(i=0; i<num_hub_levels; i++)
    if(hub_levels[i].tmpfile)
      remove(hub_levels[i].tmpfile);

  num_hub_levels = 0;
}

void P_InitHubs()
{
  num_hub_levels = 0;
}

static hublevel_t *HublevelForName(char *name)
{
  int i;

  for(i=0; i<num_hub_levels; i++)
    {
      if(!strncasecmp(name, hub_levels[i].levelname, 8))
	return &hub_levels[i];
    }

  return NULL;  // not found
}

static hublevel_t *AddHublevel(char *levelname)
{
  strncpy(hub_levels[num_hub_levels].levelname, levelname, 8);
  hub_levels[num_hub_levels].tmpfile = NULL;

  return &hub_levels[num_hub_levels++];
}

// save the current level in the hub

static void SaveHubLevel()
{
  hublevel_t *hublevel;

  hublevel = HublevelForName(levelmapname);

  // create new hublevel if not been there yet
  if(!hublevel)
    hublevel = AddHublevel(levelmapname);

  // allocate a temp. filename for save
  if(!hublevel->tmpfile)
    hublevel->tmpfile = tmpnam2();

  G_SaveCurrentLevel(hublevel->tmpfile, "smmu hubs");
}

static void LoadHubLevel(char *levelname)
{
  hublevel_t *hublevel;

  hublevel = HublevelForName(levelname);

  if(!hublevel)
    {
      // load level normally
      gamemapname = strdup(levelname);
      gameaction = ga_loadlevel;
    }
  else
    {
      // found saved level: reload
      G_LoadGame(hublevel->tmpfile, 0, 0);
      hub_changelevel = true;
    }

  wipegamestate = gamestate;
}

void P_HubChangeLevel(char *levelname)
{
  hub_changelevel = true;

  C_Printf("hubs: go to level %s\n", levelname);
  SaveHubLevel();
  LoadHubLevel(levelname);
}

void P_DumpHubs()
{
  int i;
  char tempbuf[10];

  for(i=0; i<num_hub_levels; i++)
    {
      strncpy(tempbuf, hub_levels[i].levelname, 8);
      C_Printf("%s: %s\n", tempbuf, hub_levels[i].tmpfile ?
	       hub_levels[i].tmpfile : "");
    }
}
