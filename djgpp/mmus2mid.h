// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
//-----------------------------------------------------------------------------
//
#if !defined( MMUS2MID_H )
#define MMUS2MID_H

// error codes

typedef enum
{
  MUSDATACOR,    // MUS data corrupt 
  TOOMCHAN,      // Too many channels 
  MEMALLOC,      // Memory allocation error 
  MUSDATAMT,     // MUS file empty 
  BADMUSCTL,     // MUS event 5 or 7 found 
  BADSYSEVT,     // MUS system event not in 10-14 range 
  BADCTLCHG,     // MUS control change larger than 9 
  TRACKOVF,      // MIDI track exceeds allocation 
  BADMIDHDR,     // bad midi header detected 
} error_code_t;

// some names for integers of various sizes, all unsigned 

typedef unsigned char UBYTE;  // a one-byte int 
typedef unsigned short UWORD; // a two-byte int 
typedef unsigned int ULONG;   // a four-byte int (assumes int 4 bytes) 

extern int mmus2mid(UBYTE *mus,MIDI *mid, UWORD division, int nocomp);
extern int MIDIToMidi(MIDI *mididata,UBYTE **mid,int *midlen);
extern int MidiToMIDI(UBYTE *mid,MIDI *mididata);

#endif
//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:12  fraggle
// Initial revision
//
//
//----------------------------------------------------------------------------
