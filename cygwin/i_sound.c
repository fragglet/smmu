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
//      System interface for sound.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id$";

#include <stdio.h>

#include "../c_runcmd.h"
#include "../doomstat.h"
#include "../i_sound.h"
#include "../i_system.h"
#include "../w_wad.h"
#include "../g_game.h"     //jff 1/21/98 added to use dprintf in I_RegisterSong
#include "../d_main.h"

FILE *sndpipe;

void I_CacheSound(sfxinfo_t *sound);

int snd_card = 1, mus_card = 0, detect_voices;

//
// This function loads the sound data from the WAD lump,
//  for single sound.
//
//  static void *getsfx(char *sfxname, int *len)
//  {
//    unsigned char *sfx, *paddedsfx;
//    int  i;
//    int  size;
//    int  paddedsize = 0;
//    char name[20];
//    int  sfxlump;

//    // Get the sound data from the WAD, allocate lump
//    //  in zone memory.
//    sprintf(name, "ds%s", sfxname);

//    // Now, there is a severe problem with the
//    //  sound handling, in it is not (yet/anymore)
//    //  gamemode aware. That means, sounds from
//    //  DOOM II will be requested even with DOOM
//    //  shareware.
//    // The sound list is wired into sounds.c,
//    //  which sets the external variable.
//    // I do not do runtime patches to that
//    //  variable. Instead, we will use a
//    //  default sound for replacement.

//    if ( W_CheckNumForName(name) == -1 )
//      sfxlump = W_GetNumForName("dspistol");
//    else
//      sfxlump = W_GetNumForName(name);

//    size = W_LumpLength(sfxlump);

//    sfx = W_CacheLumpNum(sfxlump, PU_STATIC);

//    // Pads the sound effect out to the mixing buffer size.
//    // The original realloc would interfere with zone memory.
//    //  paddedsize = ((size-8 + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT;

//    // Allocate from zone memory.
//    paddedsfx = (unsigned char*) Z_Malloc(paddedsize+8, PU_STATIC, 0);

//    // ddt: (unsigned char *) realloc(sfx, paddedsize+8);
//    // This should interfere with zone memory handling,
//    //  which does not kick in in the soundserver.

//    // Now copy and pad.
//    memcpy(paddedsfx, sfx, size);
//    for (i=size; i<paddedsize+8; i++)
//      paddedsfx[i] = 128;

//    // Remove the cached lump.
//    Z_Free(sfx);

//    // Preserve padded length.
//    *len = paddedsize;

//    return NULL;
//  }

// SFX API
// Note: this was called by S_Init.
// However, whatever they did in the
// old DPMS based DOS version, this
// were simply dummies in the Linux
// version.
// See soundserver initdata().
//

void I_SetChannels()
{
  // no-op.
}


void I_SetSfxVolume(int volume)
{
  // Identical to DOS.
  // Basically, this should propagate
  //  the menu/config file setting
  //  to the state variable used in
  //  the mixing.
  snd_SfxVolume = volume;
}

// jff 1/21/98 moved music volume down into MUSIC API with the rest

//
// Retrieve the raw data lump index
//  for a given SFX name.
//
int I_GetSfxLumpNum(sfxinfo_t* sfx)
{
  char namebuf[9];
  sprintf(namebuf, "ds%s", sfx->name);
  return W_CheckNumForName(namebuf);
}

// Almost all of the sound code from this point on was
// rewritten by Lee Killough, based on Chi's rough initial
// version.

// killough 2/21/98: optionally use varying pitched sounds

#define PITCH(x) (pitched_sounds ? ((x)*1000)/128 : 1000)

// This is the number of active sounds that these routines
// can handle at once, regardless of the mixer's ability
// (which we don't care about since allegro does the mixing)
// We set it to some ridiculously large number, to avoid
// any chances that these routines will stop the sounds.
// killough

#define NUM_CHANNELS 256

// "Channels" used to buffer requests. Distinct SAMPLEs
// must be used for each active sound, or else clipping
// will occur.

// static SAMPLE channel[NUM_CHANNELS];

// This function adds a sound to the list of currently
// active sounds, which is maintained as a given number
// of internal channels. Returns a handle.

int I_StartSound(sfxinfo_t *sound, int vol, int sep, int pitch, int pri)
{
  return 0;
}

// Stop the sound. Necessary to prevent runaway chainsaw,
// and to stop rocket launches when an explosion occurs.

void I_StopSound (int handle)
{
}

// Update the sound parameters. Used to control volume,
// pan, and pitch changes such as when a player turns.

void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
}

// We can pretend that any sound that we've associated a handle
// with is always playing.

int I_SoundIsPlaying(int handle)
{
  return 1;
}

// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the global
//  mixbuffer, clamping it to the allowed range,
//  and sets up everything for transferring the
//  contents of the mixbuffer to the (two)
//  hardware channels (left and right, that is).
//
//  allegro does this now

void I_UpdateSound( void )
{
}

// This would be used to write out the mixbuffer
//  during each game loop update.
// Updates sound buffer and audio device at runtime.
// It is called during Timer interrupt with SNDINTR.

void I_SubmitSound(void)
{
  //this should no longer be necessary because
  //allegro is doing all the sound mixing now
}

void I_ShutdownSound(void)
{
}

// sf: dynamic sound resource loading
void I_CacheSound(sfxinfo_t *sound)
{
}

void I_InitSound(void)
{
}

///
// MUSIC API.
//

// This is the number of active musics that these routines
// can handle at once, regardless of the mixer's ability
// (which we don't care about since allegro does the mixing)
// We set it to 1 to allow just one music at a time for now.

#define NUM_MIDICHAN 1

// mididata is used to buffer the current music.

// static MIDI mididata;

void I_ShutdownMusic(void)
{
}

void I_InitMusic(void)
{
  atexit(I_ShutdownMusic); //jff 1/16/98 enable atexit routine for shutdown
}

// jff 1/18/98 changed interface to make mididata destroyable

void I_PlaySong(int handle, int looping)
{
}

void I_SetMusicVolume(int volume)
{
  // Internal state variable.
  snd_MusicVolume = volume;
  // Now set volume on output device.

        // set volume here
}

void I_PauseSong (int handle)
{
}

void I_ResumeSong (int handle)
{
}

void I_StopSong(int handle)
{
}

void I_UnRegisterSong(int handle)
{
}

// jff 1/16/98 created to convert data to MIDI ala Allegro

int I_RegisterSong(void *data)
{
return 0;
}

// Is the song playing?
int I_QrySongPlaying(int handle)
{
  return 0;
}

/************************
        CONSOLE COMMANDS
 ************************/

// system specific sound console commands

void I_Sound_AddCommands()
{
}


//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-06-19 15:00:23  fraggle
// cygwin support
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
