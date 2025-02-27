#ifndef SOUNDFILE_H
#define SOUNDFILE_H

#include "soundspooler.h"
#include "types.h"


/*
	SoundFilePlayer

	Unless otherwise noted, all fields are read-only.
*/

#define MAX_SOUNDFILE_BUFS (8)

typedef struct SoundFilePlayer
{
	SoundSpooler* sfp_Spooler;
	char* sfp_BufferAddrs[MAX_SOUNDFILE_BUFS];
	Item    sfp_FileItem;
	Item	sfp_IOReqItem;      /* For block reads. */

	Item    sfp_SamplerIns;     /* Instrument used to play samples.

								   When SFP_NO_SAMPLER is not set, this item is set to
								   an sample player instrument loaded by the sound file player.
								   Client can access the sampler instrument to connect
								   to it, twiddle knobs, etc.  Don't write to this
								   field when SFP_NO_SAMPLER is not set.

								   When SFP_NO_SAMPLER is set, the sound file player
								   assumes that the client will write the item #
								   of a sample player instrument here between calls
								   to CreateSoundFilePlayer() and LoadSoundFile().
								   If the sound spooler is responsible for the output
								   instrument, it will connect the client-supplied sample
								   instrument to the output instrument. */

	Item    sfp_OutputIns;      /* Instrument used to output sound.

								   When SFP_NO_DIRECTOUT is not set, LoadSoundFile() loads
								   directout.dsp and places its item number here. LoadSoundFile()
								   also connects sfp_SamplerIns to sfp_OutputIns.  The
								   sound file player is responsible for starting, stopping,
								   and unloading the instrument in this case.  Don't touch it.

								   When SFP_NO_DIRECTOUT is set, no output instrument is
								   loaded, and sfp_SampleIns is not connected to anything.
								   That becomes the client's responsibility. In this case,
								   this field is set to 0. Don't touch it. */

	uint32  sfp_Cursor;         /* Position of file reader. */
	uint32  sfp_NumBuffers;     /* Number of Buffers in use. */
	uint32  sfp_BufIndex;       /* Next buffer to be loaded. */
	uint32  sfp_BufSize;        /* Size of the buffer in bytes. */
	int32   sfp_LastPos;        /* Byte position of DMA, updated periodically */
	uint32  sfp_NumToRead;      /* Number of bytes to read, total. */
	uint32  sfp_DataOffset;     /* Start of Data in file. */
	uint32  sfp_BuffersPlayed;  /* How many played so far. */
	uint32  sfp_BuffersToPlay;  /* How many total need to be played. */

	uint8	sfp_Flags;          /* User settable flags (SFP_). */

	uint8	sfp_PrivateFlags;   /* Private flags. Don't Touch! */
	uint32  sfp_BlockSize;      /* Block size of file. (V24) */
} SoundFilePlayer;


SoundFilePlayer* OpenSoundFile(char* FileName, int32 NumBuffers, int32 BufSize);
int32 CloseSoundFile(SoundFilePlayer* sfp);
int32 ServiceSoundFile(SoundFilePlayer* sfp, int32 SignalIn, int32* SignalNeeded);
int32 StartSoundFile(SoundFilePlayer* sfp, int32 Amplitude);
int32 StopSoundFile(SoundFilePlayer* sfp);

#endif
