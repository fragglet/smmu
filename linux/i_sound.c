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
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>

#include "../c_runcmd.h"
#include "../doomstat.h"
#include "../i_sound.h"
#include "../i_system.h"
#include "../w_wad.h"
#include "../g_game.h"     //jff 1/21/98 added to use dprintf in I_RegisterSong
#include "../d_main.h"

#include "i_esound.h"

// This is the number of active sounds that these routines
// can handle at once, regardless of the mixer's ability.

#define NUM_CHANNELS 16

// Padding unit for the mixer.
#define SAMPLECOUNT 1

// Sample rate and format for /dev/dsp
// TODO: Add a console command to set these
static int OUTPUTRATE = 22050;
static const int format = DSP_BITS16 | DSP_STEREO;

static int audio;	// /dev/dsp

// "Channels" used to buffer requests. Distinct SAMPLEs
// must be used for each active sound, or else clipping
// will occur.

static SAMPLE channel[NUM_CHANNELS];
static int chansUsed;	// How many channels have been used
static int activeChannels = 0; // Channels currently being mixed
static int samplesThisTic = 0; // Regulated output rate

void I_CacheSound(sfxinfo_t *sound);
static void mixFrom(SAMPLE *chan);

int snd_card = 1, mus_card = 0, detect_voices;

//This function loads the sound data from the WAD lump,
//for single sound.

static void *getsfx(char *sfxname, SAMPLE *chan)
{
  octet *sfx;
  int  size;
  char name[20];
  int  sfxlump;

  // Get the sound data from the WAD, allocate lump
  //  in zone memory.
  sprintf(name, "ds%s", sfxname);

  // Now, there is a severe problem with the
  //  sound handling, in it is not (yet/anymore)
  //  gamemode aware. That means, sounds from
  //  DOOM II will be requested even with DOOM
  //  shareware.
  // The sound list is wired into sounds.c,
  //  which sets the external variable.
  // I do not do runtime patches to that
  //  variable. Instead, we will use a
  //  default sound for replacement.

  if ( W_CheckNumForName(name) == -1 )
    sfxlump = W_GetNumForName("dspistol");
  else
    sfxlump = W_GetNumForName(name);

  size = W_LumpLength(sfxlump);

  sfx = (octet *) W_CacheLumpNum(sfxlump, PU_STATIC);
  // This should interfere with zone memory handling,
  //  which does not kick in in the soundserver.

  chan->sourceRate = sfx[2] | sfx[3] << 8;
  chan->sfx = sfx+8;
  chan->end = sfx + size;
  chan->lump = sfx;
  
  return NULL;
}

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

// This function adds a sound to the list of currently
// active sounds, which is maintained as a given number
// of internal channels. Returns a handle.

int I_StartSound(sfxinfo_t *sound, int vol, int sep, int pitch, int pri)
{
  SAMPLE *chan, *bestchan;
  int handle;
  int mindelay;

  // Try to find a free SAMPLE record
  for (chan = channel; chan < ( channel + chansUsed ); ++chan)
      if (!chan->inuse)
          break;

  if (chan >= ( channel + NUM_CHANNELS ))
    {
      // No channels free, so try to kill a sample thats nearly finished
      mindelay = 0x1000000;
      bestchan = &channel[0];
      for (chan = channel; chan < (channel + chansUsed ); ++chan)
	{
	  if (mindelay > chan->end - chan->pos)
	    {
	      bestchan = chan;
	      mindelay = chan->end - chan->pos;
	    }
	}
      chan = bestchan;
    }
  
  handle = chan - channel;
  if (chansUsed <= handle)
      chansUsed = handle+1;

  if (chan->inuse)
    {
      I_StopSound(chan->handle);
      handle = (chan->handle += NUM_CHANNELS);
    }
  else
    {
      chan->handle = handle;
    }

  // Load the sample data from the lump
  getsfx(sound->name, chan);
  chan->pos = chan->sfx;
  chan->inuse = 1;
  I_UpdateSoundParams(handle, vol, sep, pitch);

  return handle;
}

// Stop the sound. Necessary to prevent runaway chainsaw,
// and to stop rocket launches when an explosion occurs.

void I_StopSound (int handle)
{
  SAMPLE *chan = &channel[handle % NUM_CHANNELS];

  if (chan->inuse && chan->handle == handle)
    {
      chan->inuse = 0;
      //Z_Free(chan->lump); //causes segv's later for some reason
    }
}

// Update the sound parameters. Used to control volume,
// pan, and pitch changes such as when a player turns.

void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
  SAMPLE *chan;

  if (audio < 0)
    return;

  chan = &channel[handle % NUM_CHANNELS];
  if (chan->handle != handle)
    {
      // Sound was killed
      return;
    }

  chan->left_scale = (255-sep) * vol * VOLUME_BASE / (16 * 128);
  chan->right_scale = (sep) * vol * VOLUME_BASE / (16 * 128);
  chan->rate = chan->sourceRate * PITCH(pitch) / 1000;
}

// Check the 'finished' flag of the channel to find out if the
// sound is still playing.

int I_SoundIsPlaying(int handle)
{
  SAMPLE *chan = &channel[handle % NUM_CHANNELS];

  if (chan->inuse && chan->handle == handle)
    return 1;

  return 0;
}

// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the global
//  mixbuffer, clamping it to the allowed range,
//  and sets up everything for transferring the
//  contents of the mixbuffer to /dev/dsp

static int mixbuf[8192];
static short outbuf[8192];
static int mixoffset = 0; // Any samples that were missed last tic

static struct timeval thisTicTime, lastTicTime, ticDelay;

void I_UpdateSound( void )
{
  SAMPLE *chan;
  int samplesThisChan;

  // Try to regulate the audio output rate
  gettimeofday(&thisTicTime, 0);
  timersub(&thisTicTime, &lastTicTime, &ticDelay);
  lastTicTime = thisTicTime;
  if (ticDelay.tv_sec)
    {
      samplesThisTic = 8192;
    }
  else
    {
      // / 1000000 (usecs/sec), * 2 (output channels), avoiding integer
      // overflow, and add a slight speed-up to avoid breaking up samples.
      samplesThisTic = (ticDelay.tv_usec / 100 * OUTPUTRATE / 9990) * 2;

      // Do some write-ahead to avoid breaking up samples just after a mixer
      // starts up.
      if (!activeChannels)
        samplesThisTic += 2048;
    }

  // Limit the mixed data to the size of the buffers
  if (samplesThisTic > 8192)
    samplesThisTic = 8192;
  samplesThisTic -= mixoffset * 4; // Slow down if overflowing dsp

  if (samplesThisTic <= 0)
    {
      samplesThisTic = 0;
      return; // Backlog of sound data to be output
    }

  memset(mixbuf, 0, samplesThisTic * sizeof (int));
  activeChannels = 0;

  for (chan = channel; chan < channel + chansUsed ; ++chan)
    {
      if (!chan->inuse)
	continue;

      ++activeChannels;

      // Decide how much data to send for this channel
      samplesThisChan =  chan->end - chan->pos;
       
      if (samplesThisChan > 0)
	{
	  mixFrom(chan);
	}
      else
	{
	  // This sample has finished
	  I_StopSound(chan->handle);
	}
    }
}

// This would be used to write out the mixbuffer
//  during each game loop update.
// Updates sound buffer and audio device at runtime.
// It is called during Timer interrupt with SNDINTR.

void I_SubmitSound(void)
{
  int i;
  int sample;
  int written;
  int length;
  
  if (!activeChannels)
    return; // let dsp catch up if mixer is idle

  // Clamp the samples to the 16-bit range
  for (i=0; i<samplesThisTic; ++i)
    {
      sample = mixbuf[i];
      if (sample < SHRT_MIN)
        outbuf[i+mixoffset] = SHRT_MIN;
      else if (sample > SHRT_MAX)
        outbuf[i+mixoffset] = SHRT_MAX;
      else
        outbuf[i+mixoffset] = sample;
    }

  length = (samplesThisTic + mixoffset) * sizeof (short);
  if (length <= 0)
    return;

  written = write(audio, outbuf, length);
  if (written < 0)
    written = 0;

  // keep any unwritten data for the next tic
  mixoffset = (length - written) / sizeof (short);
  if (mixoffset > 0)
    {
      memmove(outbuf, outbuf+written, length - written);
    }
}

void I_ShutdownSound(void)
{
  //esd_close(control);
  close(audio);
}

// sf: dynamic sound resource loading
void I_CacheSound(sfxinfo_t *sound)
{
  // Not implemented yet, but definitely needed. - finnw
}

void I_InitSound(void)
{
  static int init = 0;
  if (init)
    return;
  init = 1;
  
  audio = audio_open(format, OUTPUTRATE);

  if (audio >= 0)
    fcntl(audio, F_SETFL, O_NONBLOCK);
  
  memset(mixbuf, 0, samplesThisTic*sizeof(int));
  memset(outbuf, 0, samplesThisTic*sizeof(short));
  mixoffset = 0;
  gettimeofday(&lastTicTime, 0);

  // Start a sound so we know its working
  I_StartSound(&S_sfx[41], 15, 128, 128, 64);
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

/************************
     Software mixing
 ************************/

static void mixFrom(SAMPLE *chan)
{
  int inp = 0, outp = 0;
  int pos;
  int frac = 0;
  int sample;

  if (!chan->inuse)
    return; // No data from this channel

  while (outp < samplesThisTic && inp < (chan->end - chan->pos - 1))
    {
      // Do smooth resampling if the sample has an unusual rate,
      // eg some in the South Park WAD.
      sample = (chan->pos[inp] - 128) * (16 - frac);
      if (frac)
        sample += (chan->pos[inp + 1] - 128) * (frac);
      sample <<= 4;
      mixbuf[outp++] += sample * chan->right_scale / VOLUME_BASE;
      mixbuf[outp++] += sample * chan->left_scale / VOLUME_BASE;
      pos = outp * chan->rate * (16/2) / OUTPUTRATE;
      inp = pos >> 4;
      frac = pos & 0xf;
    }

  chan->pos += inp + 1;
}

/************************
    DSP device setup
 ************************/
 
int audio_open(int format, int rate)
{
    // Based on the esound drivers

    const char *device = "/dev/dsp";

    int afd = -1, value = 0, test = 0;
    int mode = O_WRONLY|O_NONBLOCK|O_NOCTTY;

    /* open the sound device */
    if ((afd = open(device, mode, 0)) < 0)
    {   /* Opening device failed */
        perror(device);
        return( -1 );
    }

    /* set the sound driver audio format for playback */
    value = test = ( (format & DSP_MASK_BITS) == DSP_BITS16 )
        ? /* 16 bit */ 16 : /* 8 bit */ 8;
    if (ioctl(afd, SNDCTL_DSP_SETFMT, &test) < 0)
    {   /* Fatal error */
        perror("SNDCTL_DSP_SETFMT");
        close( afd );
        return( -1 );
    }

    ioctl(afd, SNDCTL_DSP_GETFMTS, &test);
    if ( !(value & test) ) /* TODO: should this be if ( value XOR test ) ??? */
    {   /* The device doesn't support the requested audio format. */
        fprintf( stderr, "unsupported sound format: %d\n", format );
        close( afd );
        return( -1 );
    }

    /* set the sound driver number of channels for playback */
    value = test = ( ( ( format & DSP_MASK_CHAN) == DSP_STEREO )
        ? /* stereo */ 1 : /* mono */ 0 );
    if (ioctl(afd, SNDCTL_DSP_STEREO, &test) < 0)
    {   /* Fatal error */
        perror( "SNDCTL_DSP_STEREO" );
        close( afd );
        return( -1 );
    }

    /* set the sound driver number playback rate */
    test = rate;
    if ( ioctl(afd, SNDCTL_DSP_SPEED, &test) < 0)
    { /* Fatal error */
        perror("SNDCTL_DSP_SPEED");
        close( afd );
        return( -1 );
    }

    /* see if actual speed is within 5% of requested speed */
    if( fabs( test - rate ) > rate * 0.05 )
    {
        fprintf( stderr, "unsupported playback rate: %d\n", rate );
        close( afd );
        return( -1 );
    }

    /* value = test = buf_size; */
    sleep(1); /* give the driver a chance to wake up, it's kinda finicky that way... */
    return afd;
}

//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.2  2000-07-29 22:38:24  fraggle
// linux sound support (thanks finnw)
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
