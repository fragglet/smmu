// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//      System interface for sound.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id$";

// proff 07/04/98: Changed from _MSC_VER to _WIN32 for CYGWIN32 compatibility

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
// proff 07/04/98: Added for CYGWIN32 compatibility
//#if defined (_MSC_VER) || defined (__MINGW32__)
#include <mmsystem.h>
//#endif
#include "../doomtype.h"
// proff 07/04/98: Added for CYGWIN32 compatibility
#if defined (_MSC_VER) || defined (__MINGW32__)
#define HAVE_LIBDSOUND
#define MCI_MIDI    1
#define STREAM_MIDI 2
#endif
#if defined (HAVE_LIBDSOUND) && !defined (__MINGW32__)
//#define __BYTEBOOL__
//#define false 0
//#define true !false
#endif
#if defined (HAVE_LIBDSOUND)

#include <d3dtypes.h>
#include <dsound.h>
#endif

#include "../doomdef.h"
#include "../doomstat.h"
//#include "../mmus2mid.h"
#include "../i_sound.h"
#include "../w_wad.h"
#include "../m_misc.h"
// proff 11/21/98: Added DirectSound device selection
#include "../m_argv.h"

int snd_card = 1;
int snd_freq = 1;
int snd_bits;
int snd_stereo;
int snd_dsounddevice=0;
int snd_mididevice=0;
int mus_card;
extern boolean nosfxparm, nomusicparm;
extern HWND ghWnd;
int used_mus_card;

void I_CheckMusic();

// proff 07/02/98: Moved music-varibles down to music-functions

extern  int  numChannels;
extern  int  default_numChannels;

#define DS_VOLRANGE 2000
#define DS_PANRANGE 3000
#define DS_PITCHRANGE 1000
// proff 07/09/98: Added these macros to simplify the functions
#define SEP(x)   ((x-128)*DS_PANRANGE/128)
#define VOL(x)   ((x*DS_VOLRANGE/15)-DS_VOLRANGE)
#define PITCH(x) (pitched_sounds ? ((x-128)*DS_PITCHRANGE/128) : 0)

boolean noDSound = false; //true;

// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef HAVE_LIBDSOUND

LPDIRECTSOUND lpDS;
LPDIRECTSOUNDBUFFER lpPrimaryDSB;
LPDIRECTSOUNDBUFFER *lpSecondaryDSB;
  // proff 11/21/98: Added DirectSound device selection
LPGUID DSDeviceGUIDs[16];
int DSDeviceCount=0;
int DSoundDevice=0;

typedef struct {
  sfxinfo_t *sfx;
  int samplerate;
  int endtime;
  int playing;
} channel_info_t;

channel_info_t *ChannelInfo;

static HRESULT CreateSecondaryBuffer(LPDIRECTSOUNDBUFFER *lplpDsb, int size)
{
    PCMWAVEFORMAT pcmwf;
    DSBUFFERDESC dsbdesc;

    memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
    pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
    pcmwf.wf.nChannels = 1;
    pcmwf.wf.nSamplesPerSec = 11025;
    pcmwf.wf.nBlockAlign = 1;
    pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec;
    pcmwf.wBitsPerSample = 8;

    memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);
    dsbdesc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_STATIC;
    dsbdesc.dwBufferBytes = size; 
    dsbdesc.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;

    return IDirectSound_CreateSoundBuffer(lpDS,&dsbdesc, lplpDsb, NULL);
}

static HRESULT CreatePrimaryBuffer(void)
{
  DSBUFFERDESC dsbdesc;
// proff 07/23/98: Added WAVEFORMATEX and HRESULT
  WAVEFORMATEX wf;
  HRESULT result;

  memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
  dsbdesc.dwSize = sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER; 

  memset(&wf, 0, sizeof(WAVEFORMATEX));
  if (snd_bits!=16)
    snd_bits=8;
// proff 07/23/98: Added wf
  wf.wFormatTag = WAVE_FORMAT_PCM;
  if (snd_stereo!=0)
    wf.nChannels = 2;
  else
    wf.nChannels = 1;
  wf.wBitsPerSample = snd_bits;
  wf.nSamplesPerSec = snd_freq;
  wf.nBlockAlign = wf.nChannels*wf.wBitsPerSample/8;
  wf.nAvgBytesPerSec = wf.nSamplesPerSec*wf.nBlockAlign;
    
  result=IDirectSound_CreateSoundBuffer(lpDS, &dsbdesc, &lpPrimaryDSB, NULL);
// proff 07/23/98: Added wf and result
  if (result == DS_OK)
    result=IDirectSoundBuffer_SetFormat(lpPrimaryDSB,&wf);
  if (result == DS_OK)
    result=IDirectSoundBuffer_Play(lpPrimaryDSB,0,0,DSBPLAY_LOOPING);
  return result;
}
#endif // HAVE_LIBDSOUND

void I_SetChannels()
{
}

int I_GetSfxLumpNum (sfxinfo_t* sfx)
{
  char namebuf[9];
  sprintf(namebuf, "ds%s", sfx->name);
  return W_CheckNumForName(namebuf);
}

void I_CacheSound(sfxinfo_t *sound)
{
  if(sound->data)
    return;     // already cached
  
  // sf: changed
  if(sound->link)
    I_CacheSound(sound->link);
  else {
    char name[20];
    int  sfxlump;
    
    // Get the sound data from the WAD, allocate lump
    //  in zone memory.
    sprintf(name, "ds%s", sound->name);

    sfxlump = W_GetNumForName(name);

    sound->length = W_LumpLength(sfxlump);
    sound->data = W_CacheLumpNum(sfxlump, PU_STATIC);
  }
}


void I_UpdateSoundParams(int channel, int vol, int sep, int pitch)
{
// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef HAVE_LIBDSOUND

  int DSB_Status;
  if (noDSound == true)
    return;

// proff 07/26/98: Added volume check
  if (vol==0)
    {
      IDirectSoundBuffer_Stop(lpSecondaryDSB[channel]);
      return;
    }
  IDirectSoundBuffer_SetVolume(lpSecondaryDSB[channel],VOL(vol));
  IDirectSoundBuffer_SetPan(lpSecondaryDSB[channel],SEP(sep));
  IDirectSoundBuffer_SetFrequency
    (lpSecondaryDSB[channel], ChannelInfo[channel].samplerate+PITCH(pitch));
  if (ChannelInfo[channel].playing == true)
    {
      IDirectSoundBuffer_GetStatus(lpSecondaryDSB[channel], &DSB_Status);
      if ((DSB_Status & DSBSTATUS_PLAYING) == 0)
	IDirectSoundBuffer_Play(lpSecondaryDSB[channel], 0, 0, 0);
    }
#endif // HAVE_LIBDSOUND
}

void I_StopSound(int channel)
{
// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef HAVE_LIBDSOUND
  IDirectSoundBuffer_Stop(lpSecondaryDSB[channel]);
  ChannelInfo[channel].playing=false;
#endif // HAVE_LIBDSOUND
}

int I_SoundIsPlaying(int channel)
{
// proff 07/14/98: Added this because ChannelInfo is not initialized when nosound
  if (noDSound == true)
    return false;
// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef HAVE_LIBDSOUND
// proff 10/31/98: Use accurate time for this one

  if(ChannelInfo[channel].playing &&
     (I_GetTime_RealTime() > ChannelInfo[channel].endtime))
    I_StopSound(channel);

  return ChannelInfo[channel].playing;
#else // HAVE_LIBDSOUND
  return true;
#endif // HAVE_LIBDSOUND
}

#ifdef HAVE_LIBDSOUND
// find a free channel

static int I_GetFreeChannel()
{
  int i, oldest;

  // look for a free channel slot

  for(i=0; i<numChannels; i++)
    if(!ChannelInfo[i].playing)
      return i;

  // if none free, use the oldest

  oldest = 0;

  for(i=0; i<numChannels; i++)
    if(ChannelInfo[i].endtime < ChannelInfo[oldest].endtime)
      oldest = i;

  I_StopSound(oldest);

  return oldest;
}
#endif

int I_StartSound(sfxinfo_t *sound, int vol, int sep, int pitch, int pri)
{
  int channel=0;

  // proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef HAVE_LIBDSOUND
  HRESULT error;
  char *snddata;
  int sndlength;

  if (noDSound == true)
    return channel;

  // load sound data if we have not already  

  I_CacheSound(sound);

  // find a free channel

  channel = I_GetFreeChannel();

  // proff 07/26/98: Added volume check
  // proff 10/31/98: Added Stop before updating sound-data

  error = IDirectSoundBuffer_Stop(lpSecondaryDSB[channel]);
  ChannelInfo[channel].playing = false;
  
  if (vol==0)
    return channel;

  snddata = sound->data;

  ChannelInfo[channel].samplerate = (snddata[3] << 8) + snddata[2];
// proff 10/31/98: Use accurate time for this one
  ChannelInfo[channel].endtime = 
    I_GetTime_RealTime() + 
    (sound->length * 35) / ChannelInfo[channel].samplerate + 1;

  // skip past header

  snddata += 8;
  sndlength = sound->length - 8;

  error = IDirectSoundBuffer_SetCurrentPosition(lpSecondaryDSB[channel],0);

  // proff 11/09/98: Added for a slight speedup
  if (sound != ChannelInfo[channel].sfx)
    {
      DWORD *hand1,*hand2;
      DWORD len1,len2;

      ChannelInfo[channel].sfx = sound;
      error = IDirectSoundBuffer_Lock(lpSecondaryDSB[channel],0,65535,
				      &hand1,&len1,&hand2,&len2,
				      DSBLOCK_FROMWRITECURSOR);
      if (len1 >= sndlength) 
	{
	  memset(hand1, 128, len1);
	  memcpy(hand1, snddata , sndlength);
	  memset(hand2, 128, len2);
	}
      else 
	{
	  memcpy(hand1, snddata, len1);
	  memcpy(hand2, &((char *)snddata)[len1], sndlength-len1);
	}
      
      error = IDirectSoundBuffer_Unlock
	(lpSecondaryDSB[channel], hand1, len1, hand2, len2);
    }
  IDirectSoundBuffer_SetVolume(lpSecondaryDSB[channel], VOL(vol));
  IDirectSoundBuffer_SetPan(lpSecondaryDSB[channel], SEP(sep));
  IDirectSoundBuffer_SetFrequency(lpSecondaryDSB[channel], 
				  ChannelInfo[channel].samplerate+PITCH(pitch));
  error = IDirectSoundBuffer_Play(lpSecondaryDSB[channel], 0, 0, 0);
  ChannelInfo[channel].playing = true;

#endif // HAVE_LIBDSOUND
  return channel;
}

void I_UpdateSound( void )
{
  I_CheckMusic();
}

void I_SubmitSound(void)
{
}

void I_ShutdownSound(void)
{
  printf("I_ShutdownSound: ");
// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef HAVE_LIBDSOUND
  if (lpDS)
    {
      //      IDirectSound_Release(lpDS);
      lpDS = NULL;
      printf("released DirectSound\n");
    }
#endif // HAVE_LIBDSOUND
}

#ifdef HAVE_LIBDSOUND
static HRESULT WINAPI DSEnumCallback(LPGUID lpGUID, LPCSTR lpcstrDescription,
                    LPCSTR lpcstrModule, LPVOID lpContext)
{
  if (DSDeviceCount==16)
    return FALSE;
  printf("  Device %i: %s\n",DSDeviceCount,lpcstrDescription);
  DSDeviceGUIDs[DSDeviceCount]=lpGUID;
  DSDeviceCount++;
  return TRUE;
}
#endif // HAVE_LIBDSOUND

void I_InitSound(void)
{
// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef HAVE_LIBDSOUND
  HRESULT error;
  int c;
  int i;
#endif // HAVE_LIBDSOUND
  
  // proff 07/01/98: Added I_InitMusic
  if (!nomusicparm)
    I_InitMusic();
  if (nosfxparm)
    return;
  // proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef HAVE_LIBDSOUND
  // proff 11/21/98: Added DirectSound device selection
  i = M_CheckParm ("-dsounddevice");
  if ((i) || (snd_dsounddevice))
    {
      printf("I_InitSound: Sound Devices\n");
      DirectSoundEnumerate(&DSEnumCallback,NULL);
      DSoundDevice=snd_dsounddevice;
      if (i)
	DSoundDevice=atoi(myargv[i+1]);
      DSoundDevice=
	(DSoundDevice < 0) ? 0 : 
	(DSoundDevice >= DSDeviceCount) ? 0 : DSoundDevice;
    }

  printf("I_InitSound: ");
  error = DirectSoundCreate(DSDeviceGUIDs[DSoundDevice], &lpDS, NULL);
  // proff 11/21/98: End of additions and changes
  if (error == DSERR_NODRIVER)
    {
      lpDS = NULL;
      printf("no sounddevice found\n");
      noDSound = true;
      return;
    }
  noDSound = false;
  if (error != DS_OK)
    {
      noDSound = true;
      return;
    }
  printf("created DirectSound. Selected Device: %i\n", DSoundDevice);
  atexit(I_ShutdownSound); 
  error = IDirectSound_SetCooperativeLevel(lpDS, ghWnd, DSSCL_EXCLUSIVE);
  if (error != DS_OK)
    {
      noDSound = true;
      return;
    }
  printf("I_InitSound: ");
  printf("CooperativeLevel set\n");
  printf("I_InitSound: ");

  error = CreatePrimaryBuffer();
  //  if (error != DS_OK)
  //    {
  //      printf("ERROR: Couldnt create primary buffer\n");
  //      noDSound = true;
  //      return;
  //    }

  numChannels = default_numChannels;
  lpSecondaryDSB = malloc(sizeof(LPDIRECTSOUNDBUFFER)*numChannels);
  if (lpSecondaryDSB)
    {
      memset(lpSecondaryDSB, 0, sizeof(LPDIRECTSOUNDBUFFER) * numChannels);
      printf ("Channels : %i\n", numChannels);
    }
  else
    {
      noDSound = true;
      return;
    }
  ChannelInfo = malloc(sizeof(channel_info_t)*numChannels);

  if (ChannelInfo)
    {
      // proff 11/09/98: Added for security

      memset(ChannelInfo, 0, sizeof(channel_info_t) * numChannels);
    }
  else
    {
      noDSound = true;
      return;
    }

  for (c=0; c<numChannels; c++)
    {
      error = CreateSecondaryBuffer(&lpSecondaryDSB[c], 65535);
      if (error != DS_OK)
	{
	  noDSound = true;
	  return;
	}
    }

  printf("DirectSound initialised ok!\n");

#endif // HAVE_LIBDSOUND
}

// proff: 07/26/98: changed to static
static int MusicLoaded=0;
static int MusicLoop=0;

#ifdef STREAM_MIDI

static MIDI mididata;

static UINT MidiDevice;
static HMIDISTRM hMidiStream;
static MIDIEVENT *MidiEvents[MIDI_TRACKS];
static MIDIHDR MidiStreamHdr;
static MIDIEVENT *NewEvents;
static int NewSize;
static int NewPos;
static int BytesRecorded[MIDI_TRACKS];
static int BufferSize[MIDI_TRACKS];
static int CurrentTrack;
static int CurrentPos;

static int getvl(void)
{
  int l=0;
  byte c;
  for (;;)
  {
    c=mididata.track[CurrentTrack].data[CurrentPos];
    CurrentPos++;
    l += (c & 0x7f);
    if (!(c & 0x80)) 
      return l;
    l<<=7;
  }
}

static void AddEvent(DWORD at, DWORD type, byte event, byte a, byte b)
{
  MIDIEVENT *CurEvent;

  if ((BytesRecorded[CurrentTrack]+(int)sizeof(MIDIEVENT))>=BufferSize[CurrentTrack])
  {
    BufferSize[CurrentTrack]+=100*sizeof(MIDIEVENT);
    MidiEvents[CurrentTrack]=realloc(MidiEvents[CurrentTrack],BufferSize[CurrentTrack]);
  }
  CurEvent=(MIDIEVENT *)((byte *)MidiEvents[CurrentTrack]+BytesRecorded[CurrentTrack]);
  memset(CurEvent,0,sizeof(MIDIEVENT));
  CurEvent->dwDeltaTime=at;
  CurEvent->dwEvent=event+(a<<8)+(b<<16)+(type<<24);
  BytesRecorded[CurrentTrack]+=3*sizeof(DWORD);
}

static void MidiTracktoStream(void)
{
  DWORD atime,len;
  byte event,type,a,b,c;
  byte laststatus,lastchan;

  CurrentPos=0;
  laststatus=0;
  lastchan=0;
  atime=0;
  for (;;)
  {
    if (CurrentPos>=mididata.track[CurrentTrack].len)
      return;
    atime+=getvl();
    event=mididata.track[CurrentTrack].data[CurrentPos];
    CurrentPos++;
    if(event==0xF0 || event == 0xF7) /* SysEx event */
    {
      len=getvl();
      CurrentPos+=len;
    }
    else if(event==0xFF) /* Meta event */
    {
      type=mididata.track[CurrentTrack].data[CurrentPos];
      CurrentPos++;
      len=getvl();
      switch(type)
        {
        case 0x2f:
          return;
        case 0x51: /* Tempo */
          a=mididata.track[CurrentTrack].data[CurrentPos];
          CurrentPos++;
          b=mididata.track[CurrentTrack].data[CurrentPos];
          CurrentPos++;
          c=mididata.track[CurrentTrack].data[CurrentPos];
          CurrentPos++;
          AddEvent(atime, MEVT_TEMPO, c, b, a);
          break;
        default:
          CurrentPos+=len;
          break;
        }
    }
    else
    {
      a=event;
      if (a & 0x80) /* status byte */
      {
        lastchan=a & 0x0F;
        laststatus=(a>>4) & 0x07;
        a=mididata.track[CurrentTrack].data[CurrentPos];
        CurrentPos++;
        a &= 0x7F;
      }
      switch(laststatus)
      {
      case 0: /* Note off */
        b=mididata.track[CurrentTrack].data[CurrentPos];
        CurrentPos++;
        b &= 0x7F;
        AddEvent(atime, MEVT_SHORTMSG, (byte)((laststatus<<4)+lastchan+0x80), a, b);
        break;

      case 1: /* Note on */
        b=mididata.track[CurrentTrack].data[CurrentPos];
        CurrentPos++;
        b &= 0x7F;
        AddEvent(atime, MEVT_SHORTMSG, (byte)((laststatus<<4)+lastchan+0x80), a, b);
        break;

      case 2: /* Key Pressure */
        b=mididata.track[CurrentTrack].data[CurrentPos];
        CurrentPos++;
        b &= 0x7F;
        AddEvent(atime, MEVT_SHORTMSG, (byte)((laststatus<<4)+lastchan+0x80), a, b);
        break;

      case 3: /* Control change */
        b=mididata.track[CurrentTrack].data[CurrentPos];
        CurrentPos++;
        b &= 0x7F;
        AddEvent(atime, MEVT_SHORTMSG, (byte)((laststatus<<4)+lastchan+0x80), a, b);
        break;

      case 4: /* Program change */
        a &= 0x7f;
        AddEvent(atime, MEVT_SHORTMSG, (byte)((laststatus<<4)+lastchan+0x80), a, 0);
        break;

      case 5: /* Channel pressure */
        a &= 0x7f;
        AddEvent(atime, MEVT_SHORTMSG, (byte)((laststatus<<4)+lastchan+0x80), a, 0);
        break;

      case 6: /* Pitch wheel */
        b=mididata.track[CurrentTrack].data[CurrentPos];
        CurrentPos++;
        b &= 0x7F;
        AddEvent(atime, MEVT_SHORTMSG, (byte)((laststatus<<4)+lastchan+0x80), a, b);
        break;

      default: 
        break;
      }
    }
  }
}

static void BlockOut(void)
{
  MMRESULT err;
  int BlockSize;

  if ((MusicLoaded) && (NewEvents))
  {
    // proff 12/8/98: Added for savety
    midiOutUnprepareHeader(hMidiStream,&MidiStreamHdr,sizeof(MIDIHDR));
    if (NewPos>=NewSize)
      if (MusicLoop)
        NewPos=0;
      else 
        return;
    BlockSize=(NewSize-NewPos);
    if (BlockSize<=0)
      return;
    if (BlockSize>36000)
      BlockSize=36000;
    MidiStreamHdr.lpData=(void *)((byte *)NewEvents+NewPos);
    NewPos+=BlockSize;
    MidiStreamHdr.dwBufferLength=BlockSize;
    MidiStreamHdr.dwBytesRecorded=BlockSize;
    MidiStreamHdr.dwFlags=0;
//    lprintf(LO_DEBUG,"Data: %p, Size: %i\n",MidiStreamHdr.lpData,BlockSize);
    err=midiOutPrepareHeader(hMidiStream,&MidiStreamHdr,sizeof(MIDIHDR));
    if (err!=MMSYSERR_NOERROR)
      return;
    err=midiStreamOut(hMidiStream,&MidiStreamHdr,sizeof(MIDIHDR));
      return;
  }
}

static void MIDItoStream(void)
{
  int BufferPos[MIDI_TRACKS];
  MIDIEVENT *CurEvent;
  MIDIEVENT *NewEvent;
  int lTime;
  int Dummy;
  int Track;

  if (!hMidiStream)
    return;
  NewSize=0;
  for (CurrentTrack=0;CurrentTrack<MIDI_TRACKS;CurrentTrack++)
  {
    MidiEvents[CurrentTrack]=NULL;
    BytesRecorded[CurrentTrack]=0;
    BufferSize[CurrentTrack]=0;
    MidiTracktoStream();
    NewSize+=BytesRecorded[CurrentTrack];
    BufferPos[CurrentTrack]=0;
  }
  NewEvents=realloc(NewEvents,NewSize);
  if (NewEvents)
  {
    NewPos=0;
    while (1)
    {
      lTime=INT_MAX;
      Track=-1;
      for (CurrentTrack=MIDI_TRACKS-1;CurrentTrack>=0;CurrentTrack--)
      {
        if ((BytesRecorded[CurrentTrack]>0) && (BufferPos[CurrentTrack]<BytesRecorded[CurrentTrack]))
          CurEvent=(MIDIEVENT *)((byte *)MidiEvents[CurrentTrack]+BufferPos[CurrentTrack]);
        else 
          continue;
        if ((int)CurEvent->dwDeltaTime<=lTime)
        {
          lTime=CurEvent->dwDeltaTime;
          Track=CurrentTrack;
        }
      }
      if (Track==-1)
        break;
      else
      {
        CurEvent=(MIDIEVENT *)((byte *)MidiEvents[Track]+BufferPos[Track]);
        BufferPos[Track]+=12;
        NewEvent=(MIDIEVENT *)((byte *)NewEvents+NewPos);
        memcpy(NewEvent,CurEvent,12);
        NewPos+=12;
      }
    }
    NewPos=0;
    lTime=0;
    while (NewPos<NewSize)
    {
      NewEvent=(MIDIEVENT *)((byte *)NewEvents+NewPos);
      Dummy=NewEvent->dwDeltaTime;
      NewEvent->dwDeltaTime-=lTime;
      lTime=Dummy;
      NewPos+=12;
    }
    NewPos=0;
    MusicLoaded=1;
    BlockOut();
  }
  for (CurrentTrack=0;CurrentTrack<MIDI_TRACKS;CurrentTrack++)
  {
    if (MidiEvents[CurrentTrack])
      free(MidiEvents[CurrentTrack]);
  }
}

void CALLBACK MidiProc( HMIDIIN hMidi, UINT uMsg, DWORD dwInstance,
                        DWORD dwParam1, DWORD dwParam2 )
{
    switch( uMsg )
    {
    case MOM_DONE:
      if ((MusicLoaded) && ((DWORD)dwParam1 == (DWORD)&MidiStreamHdr))
      {
        BlockOut();
      }
      break;
    default:
      break;
    }
}
#endif // STREAM_MIDI

#ifdef MCI_MIDI
char *SafemciSendString(char *cmd)
{
  static char errora[256];
  int err;

  err = mciSendString(cmd,errora,255,NULL);
  if (err)
  {
    mciGetErrorString(err,errora,256);
        lprintf (LO_DEBUG, "%s\n",errora);
    }
    return errora;
}
#endif // MCI_MIDI

void I_CheckMusic()
{
#ifdef MCI_MIDI
  static int nexttic=0;

  if (used_mus_card!=MCI_MIDI)
    return;
  if (snd_MusicVolume==0)
    return;
  if ((gametic>nexttic) & (MusicLoaded))
  {
    nexttic=gametic+150;
    if(MusicLoop)
      if (_stricmp(SafemciSendString("status doommusic mode"),"stopped")==0)
        SafemciSendString("play doommusic from 0");
  }
#endif // MCI_MIDI
}

void I_PlaySong(int handle, int looping)
{
  if (snd_MusicVolume==0)
    return;
  switch (used_mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    if ((handle>=0) & (MusicLoaded))
    {
      if (_stricmp(SafemciSendString("status doommusic mode"),"playing")==0)
        return;
      SafemciSendString("play doommusic from 0");
      MusicLoop=looping;
    }
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    if (hMidiStream)
    {
      MusicLoop=looping;
      midiStreamRestart(hMidiStream);
    }
    break;
#endif
  default:
    break;
  }
}

void I_SetMusicVolume(int volume)
{
  snd_MusicVolume = volume;
  switch (used_mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    if (snd_MusicVolume == 0)
      I_StopSong(1);
    else
    {
      I_PlaySong(1,MusicLoop);
      if (paused)
        I_PauseSong(1);
    }
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    if (snd_MusicVolume == 0)
      I_StopSong(1);
    else
    {
      I_PlaySong(1,MusicLoop);
      if (paused)
        I_PauseSong(1);
    }
    break;
#endif
  default:
    break;
  }
}

void I_PauseSong(int handle)
{
  if (paused == 0)
    return;
  switch (used_mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    if ((handle>=0) & (MusicLoaded))
      SafemciSendString("pause doommusic");
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    if (hMidiStream)
      midiStreamPause(hMidiStream);
    break;
#endif
  default:
    break;
  }
}

void I_ResumeSong(int handle)
{
  if (paused == 1)
    return;
  switch (used_mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    if ((handle>=0) & (MusicLoaded))
      SafemciSendString("resume doommusic");
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    if (hMidiStream)
      midiStreamRestart(hMidiStream);
    break;
#endif
  default:
    break;
  }
}

void I_StopSong(int handle)
{
  switch (used_mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    if ((handle>=0) & (MusicLoaded))
      SafemciSendString("stop doommusic");
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    if (!hMidiStream)
      return;
//    midiStreamPause(hMidiStream);
    midiStreamStop(hMidiStream);
    midiOutReset(hMidiStream);
    break;
#endif
  default:
    break;
  }
}

void I_UnRegisterSong(int handle)
{
  switch (used_mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    MusicLoaded=0;
    SafemciSendString("close doommusic");
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    if (hMidiStream)
    {
      MusicLoaded=0;
      midiStreamStop(hMidiStream);
      midiOutReset(hMidiStream);
      midiStreamClose(hMidiStream);
    }
    break;
#endif
  default:
    break;
  }
}

int I_RegisterSong(void *data)
{
#ifdef MCI_MIDI
  UBYTE *mid;
  int midlen;
  char fname[PATH_MAX+1];
  char mcistring[PATH_MAX+1];
  char *D_DoomExeDir(void);
#endif
#ifdef STREAM_MIDI
  MMRESULT merr;
  MIDIPROPTIMEDIV mptd;
#endif
  /*
  int err;

//  SafemciSendString("close doommusic");
  if    //jff 02/08/98 add native midi support
    (
     (err=MidiToMIDI(data, &mididata)) &&       // try midi first
     (err=mmus2mid(data, &mididata, 89, 0))     // now try mus
     )
    {
    dprintf("Error loading midi: %d",err);
    return -1;
    }
  */
  switch (used_mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    MIDIToMidi(&mididata,&mid,&midlen);
// proff 07/01/98: Changed filename to prboom.mid
    M_WriteFile(strcat(strcpy(fname,D_DoomExeDir()),"prboom.mid"),mid,midlen);
    sprintf(mcistring,"open %s alias doommusic",fname);
    SafemciSendString(mcistring);
    free(mid);
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    memset(&MidiStreamHdr,0,sizeof(MIDIHDR));
    merr=midiStreamOpen(&hMidiStream,&MidiDevice,1,(DWORD)&MidiProc,0,CALLBACK_FUNCTION);
    if (merr!=MMSYSERR_NOERROR)
      hMidiStream=0;
    if (!hMidiStream)
      return 0;
    mptd.cbStruct=sizeof(MIDIPROPTIMEDIV);
    mptd.dwTimeDiv=mididata.divisions;
    merr=midiStreamProperty(hMidiStream,(LPBYTE)&mptd,MIDIPROP_SET | MIDIPROP_TIMEDIV);
    MIDItoStream();
    break;
#endif
  default:
    break;
  }
  MusicLoaded=1;

  return 0;
}

void I_ShutdownMusic(void)
{
  I_StopSong(1);
}

void I_InitMusic(void)
{
#ifdef STREAM_MIDI
  int i;
  int MidiDeviceCount;
  MIDIOUTCAPS MidiOutCaps;
#endif

  switch (mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    lprintf (LO_INFO, "I_InitMusic: Using MCI-MIDI Device\n");
    used_mus_card=MCI_MIDI;
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    printf ("I_InitMusic: Using Stream-MIDI Device\n");
    MidiDeviceCount=midiOutGetNumDevs();
    if (MidiDeviceCount<=0)
    {
      used_mus_card=-1;
      break;
    }
    i = M_CheckParm ("-mididevice");
    if ((i) || (snd_mididevice>0))
    {
      lprintf(LO_INFO, "I_InitMusic: Available MidiDevices\n");
      for (i=-1; i<MidiDeviceCount; i++)
        if (midiOutGetDevCaps(i,&MidiOutCaps,sizeof(MIDIOUTCAPS))==MMSYSERR_NOERROR)
          printf( "  Device %i: %s\n",i,MidiOutCaps.szPname);
        else
          printf("  Device %i: Error\n",i);
      MidiDevice=MIDI_MAPPER;
      if (snd_mididevice>0)
        MidiDevice=snd_mididevice-1;
      if (i)
        MidiDevice=atoi(myargv[i+1]);
      MidiDevice=(MidiDevice<-1) ? MIDI_MAPPER : ((MidiDevice>=(UINT)MidiDeviceCount) ? MIDI_MAPPER : MidiDevice);
    }
    used_mus_card=STREAM_MIDI;
    break;
#endif
  default:
    printf ("I_InitMusic: Using No MIDI Device\n");
    used_mus_card=-1;
    break;
  }
  atexit(I_ShutdownMusic);
}

void I_Sound_AddCommands()
{
}


//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.2  2001-01-13 02:28:25  fraggle
// changed library #defines to standard HAVE_LIBxyz
// for autoconfing
//
// Revision 1.1  2000/04/09 18:03:40  proff_fs
// Initial revision
//
// Revision 1.16  1998/09/07  20:06:36  jim
// Added logical output routine
//
// Revision 1.15  1998/05/03  22:32:33  killough
// beautification, use new headers/decls
//
// Revision 1.14  1998/03/09  07:11:29  killough
// Lock sound sample data
//
// Revision 1.13  1998/03/05  00:58:46  jim
// fixed autodetect not allowed in allegro detect routines
//
// Revision 1.12  1998/03/04  11:51:37  jim
// Detect voices in sound init
//
// Revision 1.11  1998/03/02  11:30:09  killough
// Make missing sound lumps non-fatal
//
// Revision 1.10  1998/02/23  04:26:44  killough
// Add variable pitched sound support
//
// Revision 1.9  1998/02/09  02:59:51  killough
// Add sound sample locks
//
// Revision 1.8  1998/02/08  15:15:51  jim
// Added native midi support
//
// Revision 1.7  1998/01/26  19:23:27  phares
// First rev with no ^Ms
//
// Revision 1.6  1998/01/23  02:43:07  jim
// Fixed failure to not register I_ShutdownSound with atexit on install_sound error
//
// Revision 1.4  1998/01/23  00:29:12  killough
// Fix SSG reload by using frequency stored in lump
//
// Revision 1.3  1998/01/22  05:55:12  killough
// Removed dead past changes, changed destroy_sample to stop_sample
//
// Revision 1.2  1998/01/21  16:56:18  jim
// Music fixed, defaults for cards added
//
// Revision 1.1.1.1  1998/01/19  14:02:57  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
