// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Hubs.
//
// Header file.
//
// By Simon Howard
//
//---------------------------------------------------------------------------

#ifndef __P_HUBS_H__
#define __P_HUBS_H__

void P_HubChangeLevel(char *levelname);
void P_InitHubs();
void P_ClearHubs();

void P_SavePlayerPosition(player_t *player, int sectag);
void P_RestorePlayerPosition();

void P_HubReborn();

extern boolean hub_changelevel;

#endif
