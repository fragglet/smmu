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

extern int mmus2mid(unsigned char *mus, MIDI *mid, unsigned short division,
		    int nocomp);
extern int MIDIToMidi(MIDI *mididata, unsigned char **mid,int *midlen);
extern int MidiToMIDI(unsigned char *mid, MIDI *mididata);

#endif
//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.3  2001-01-15 01:40:06  fraggle
// remove conflicting mmus2mid typedefs
//
// Revision 1.2  2001/01/15 01:34:33  fraggle
// fix ULONG typedef - need to remove this if possible to get rid of the
// conflicts with the windows.h version
//
// Revision 1.1  2001/01/14 21:08:01  fraggle
// Move mmus2mid to system-nonspecific so other platforms can use it
//
// Revision 1.1.1.1  2000/04/30 19:12:12  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
