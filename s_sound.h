// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: s_sound.h,v 1.4 1998/05/03 22:57:36 killough Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
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
void S_Startsfxinfo(const mobj_t *origin, sfxinfo_t *sfx);

// Stop sound for thing at <origin>
void S_StopSound(const mobj_t *origin);

// Start music using <music_id> from sounds.h
void S_StartMusic(int music_id);

// Start music using <music_id> from sounds.h, and set whether looping
void S_ChangeMusic(int music_id, int looping);

// Stops the music fer sure.
void S_StopMusic(void);
void S_StopSounds();

// Stop and resume music, during game PAUSE.
void S_PauseSound(void);
void S_ResumeSound(void);

sfxinfo_t *S_sfxinfoForname(char *name);
void S_UpdateSound(int lumpnum);
void S_Chgun();


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
// $Log: s_sound.h,v $
// Revision 1.4  1998/05/03  22:57:36  killough
// beautification, add external declarations
//
// Revision 1.3  1998/04/27  01:47:32  killough
// Fix pickups silencing player weapons
//
// Revision 1.2  1998/01/26  19:27:51  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:09  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
