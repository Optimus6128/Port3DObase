#include "form3DO.h"

#include "hardware.h"

#include <stdio.h>

char* GetChunk(uint32* chunk_ID, char** buffer, int32* bufLen)
{
	char* buf;
	uint32 length;

	if (*bufLen > 0) {

		buf = *buffer;
		//*chunk_ID = LONG_ENDIAN_FLIP(((PixelChunk*)buf)->chunk_ID);	// that would trigger microsoft defender false positive for a virus!!!! Now I flip the chunk defines in funcs3DO.h instead
		*chunk_ID = ((PixelChunk*)buf)->chunk_ID;

		/* jump to next chunk */
		length = ((PixelChunk*)buf)->chunk_size;
		#ifndef BIG_ENDIAN
			length = LONG_ENDIAN_FLIP(length);
		#endif

		*bufLen -= length;
		*buffer += length;

		return buf;
	}

	return NULL;
}
