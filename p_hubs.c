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
#include "p_maputl.h"
#include "p_setup.h"
#include "p_spec.h"
#include "t_vari.h"
#include "z_zone.h"

#define MAXHUBLEVELS 128

typedef struct hublevel_s hublevel_t;
struct hublevel_s
{
  char levelname[8];
  char *tmpfile;        // temporary file holding the saved level
};

extern char *gamemapname;

// sf: set when we are changing to
//  another level in the hub
boolean hub_changelevel = false;  

hublevel_t hub_levels[MAXHUBLEVELS];
int num_hub_levels;

// sf: my own tmpnam (djgpp one doesn't work as i want it)

char *temp_hubfile()
{
  static int tmpfilenum = 0;
  char *new_tmpfilename;

  new_tmpfilename = malloc(10);

  sprintf(new_tmpfilename, "smmu%i.tmp", tmpfilenum++);

  return new_tmpfilename;  
}

void P_ClearHubs()
{
  int i;

  for(i=0; i<num_hub_levels; i++)
    if(hub_levels[i].tmpfile)
      remove(hub_levels[i].tmpfile);

  num_hub_levels = 0;

  // clear the hub_script
  T_ClearHubScript();
}

// seperate function: ensure that atexit is not set twice

void P_ClearHubsAtExit()
{
  static boolean atexit_set = false;

  if(atexit_set) return;   // already set

  atexit(P_ClearHubs);

  atexit_set = true;
}

void P_InitHubs()
{
  num_hub_levels = 0;

  P_ClearHubsAtExit();    // set P_ClearHubs to be called at exit
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
    hublevel->tmpfile = temp_hubfile();

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

void P_HubReborn()
{
  // called when player is reborn when using hubs
  LoadHubLevel(levelmapname);
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

static fixed_t          save_xoffset;
static fixed_t          save_yoffset;
static mobj_t           save_mobj;
static int              save_sectag;
static player_t *       save_player;
static pspdef_t         save_psprites[NUMPSPRITES];

// save a player's position relative to a particular sector
void P_SavePlayerPosition(player_t *player, int sectag)
{
  sector_t *sec;
  int secnum;

  save_player = player;

  // save psprites whatever happens

  memcpy(save_psprites, player->psprites, sizeof(player->psprites));

  // save sector x,y offset

  save_sectag = sectag;

  C_Printf("sectag: %i\n", sectag);

  if((secnum = P_FindSectorFromTag(sectag, -1)) < 0)
    {
      // invalid: sector not found
      save_sectag = -1;
      return;
    }
  
  sec = &sectors[secnum];

  // use soundorg x and y as 'centre' of sector

  save_xoffset = player->mo->x - sec->soundorg.x;
  save_yoffset = player->mo->y - sec->soundorg.y;

  // save mobj so we can restore various bits of data

  memcpy(&save_mobj, player->mo, sizeof(mobj_t));

}

// restore the players position -- sector must be the same shape
void P_RestorePlayerPosition()
{
  sector_t *sec;
  int secnum;

  // we always save and restore the psprites

  memcpy(save_player->psprites, save_psprites, sizeof(save_player->psprites));

  // restore player position from x,y offset

  if(save_sectag == -1) return;      // no sector relativeness

  if((secnum = P_FindSectorFromTag(save_sectag, -1)) < 0)
    {
      // invalid: sector not found
      return;
    }
  
  sec = &sectors[secnum];

  // restore position

  P_UnsetThingPosition(save_player->mo);

  save_player->mo->x = sec->soundorg.x + save_xoffset;
  save_player->mo->y = sec->soundorg.y + save_yoffset;

  // restore various other things
  save_player->mo->angle = save_mobj.angle;
  save_player->mo->momx = save_mobj.momx;    // keep momentum
  save_player->mo->momy = save_mobj.momy;

  P_SetThingPosition(save_player->mo);
}
