// Could use an autoconf script
#ifdef HAVE_MACHINE_SOUNDCARD_H
#  include <machine/soundcard.h>
#else
#  ifdef HAVE_SOUNDCARD_H
#    include <soundcard.h>
#  else
#    include <sys/soundcard.h>
#  endif
#endif

typedef unsigned char octet;

#define VOLUME_BASE (256)

/* bits of stream/sample data */
#define DSP_MASK_BITS	( 0x000F )
#define DSP_BITS8 	( 0x0000 )
#define DSP_BITS16	( 0x0001 )

/* how many interleaved channels of data */
#define DSP_MASK_CHAN	( 0x00F0 )
#define DSP_MONO	( 0x0010 )
#define DSP_STEREO	( 0x0020 )

// Local sample, played through streams as necessary
typedef struct sample_s {
  int inuse;		// Is this channel in use
  int handle;		// Handle assigned for s_sound.c
  
  int sourceRate;	// Default playback rate, from the sound lump
  int rate;		// Current playback rate, possibly adjusted
  
  octet *sfx;		// The sample data, excluding the header
  octet *pos;		// The current play position
  octet *end;		// End of the sample
  
  void *lump;		// The complete cached lump
  
  int left_scale;	// Volumes for each output channel
  int right_scale;

  //struct sample_s *prev, *next;
} SAMPLE;
