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
//      The not so system specific sound interface.
//
//-----------------------------------------------------------------------------

#ifndef __S_SOUND__
#define __S_SOUND__

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void S_Init(int sfxVolume, int musicVolume);

//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_Start(void);

//
// Start sound for thing at <origin>
//  using <sound_id> from sounds.h
//
void S_StartSound(const mobj_t *origin, int sound_id);
void S_StartSoundName(const mobj_t *origin, char *name);
void S_StartSfxInfo(const mobj_t *origin, sfxinfo_t *sfx);

// Stop sound for thing at <origin>
void S_StopSound(const mobj_t *origin);

// Start music using <music_id> from sounds.h
void S_StartMusic(int music_id);
void S_StartRandomMusic();

// Start music using <music_id> from sounds.h, and set whether looping
void S_ChangeMusicNum(int music_id, int looping);
void S_ChangeMusicName(char *name, int looping);
void S_ChangeMusic(musicinfo_t *music, int looping);

// Stops the music fer sure.
void S_StopMusic(void);
void S_StopSounds();

// Stop and resume music, during game PAUSE.
void S_PauseSound(void);
void S_ResumeSound(void);

sfxinfo_t *S_SfxInfoForName(char *name);
void S_UpdateSound(int lumpnum);
void S_Chgun();

musicinfo_t *S_MusicForName(char *name);
void S_UpdateMusic(int lumpnum);

//
// Updates music & sounds
//
void S_UpdateSounds(const mobj_t *listener);
void S_SetMusicVolume(int volume);
void S_SetSfxVolume(int volume);

// precache sound?
extern int s_precache;

// machine-independent sound params
extern int numChannels;
extern int default_numChannels;  // killough 10/98

//jff 3/17/98 holds last IDMUS number, or -1
extern int idmusnum;

#endif

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:09  fraggle
// Initial revision
//
//
//----------------------------------------------------------------------------
