#ifndef SOUNDSPOOLER_H
#define SOUNDSPOOLER_H

#include "types.h"

// Dummy add structs as we don't need (yet) to precisely emulate them

typedef struct SoundSpooler
{
	//List	sspl_FreeBuffers;   /* List of SoundBufferNodes available for use. */
	//List	sspl_ActiveBuffers; /* List of SoundBufferNodes queued up in audiofolio. */
	int32   sspl_NumBuffers;
	int32   sspl_SignalMask;    /* OR of all Cue signals. */
	Item    sspl_SamplerIns;    /* Appropriate sample player instrument */
	uint32  sspl_Flags;         /* Private flags field. */
	/* Optional callback function called for SoundBufferNode state changes */
	//SoundBufferFunc sspl_SoundBufferFunc;
	//List    sspl_RequestedBuffers; /* List of SoundBufferNodes that have been returned by ssplRequestBuffer() but not resubmitted or unrequested. */
} SoundSpooler;

#endif
