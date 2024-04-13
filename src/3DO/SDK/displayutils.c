#include "displayutils.h"
#include "graphics.h"
#include "mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Item CreateBasicDisplay(ScreenContext* sc, uint32 displayType, uint32 numScreens)
{
	uint32 i;

	sc->sc_DisplayType = displayType;
	sc->sc_NumScreens = numScreens;
	sc->sc_CurrentScreen = 0;

	sc->sc_BitmapWidth = 320;	// For now it's 320x240
	sc->sc_BitmapHeight = 240;	// In the future I can set it based on type

	for (i = 0; i < numScreens; ++i) {
		sc->sc_Bitmaps[i] = malloc(sizeof(Bitmap));
		sc->sc_Bitmaps[i]->bm_Width = sc->sc_BitmapWidth;
		sc->sc_Bitmaps[i]->bm_Height = sc->sc_BitmapHeight;
		sc->sc_Bitmaps[i]->bm_Buffer = malloc(sc->sc_BitmapWidth * sc->sc_BitmapHeight * 2);	// 16bpp
		sc->sc_BitmapItems[i] = (int32)sc->sc_Bitmaps[i];	// this is a hack for the port. Normally I don't know what Item holds, possibly an id/handle to any kind of 3DO object. But now I only need this to get a pointer to the sc_Bitmap for the DrawCels.
		sc->sc_ScreenItems[i] = sc->sc_BitmapItems[i];		// Same Hack for DisplayScreen
	}

	initVDLmap();

	return 0;
}

void DeleteBasicDisplay(ScreenContext *sc)
{
	// Dummy
}

static void correctPixelDataEndianess(uint16* pData, int count)
{
	if (!pData || count <= 0) return;

	while (count-- > 0) {
		const uint16 c = *pData;
		*pData++ = SHORT_ENDIAN_FLIP(c);
	}
}

static void* parseImg(void* buffer, int32 size, ubyte* dest, ScreenContext* sc)
{
	int32 tempSize;
	char* tempBuf;
	char* pChunk;
	uint32 chunk_ID;

	ImageCC* pIMAG = NULL;
	PixelChunk* pPDAT = NULL;
	VdlChunk* pVDL = NULL;
	int32 destbufsize = 0;

	//destbufsize = sc->sc_NumBitmapBytes;
	destbufsize = sc->sc_BitmapWidth * sc->sc_BitmapHeight * 2;	// hack for now

	if (dest == NULL) {
		dest = (ubyte*)AllocMem(destbufsize, MEMTYPE_TRACKSIZE | MEMTYPE_STARTPAGE | MEMTYPE_VRAM | sc->sc_BitmapBank);
		if (dest == NULL) {
			return NULL;
		}
	}

	tempBuf = (char*)buffer;
	tempSize = size;

	while ((pChunk = GetChunk(&chunk_ID, &tempBuf, &tempSize)) != NULL) {
		switch (chunk_ID) {
		case CHUNK_IMAG:
			if (pIMAG == NULL) {
				pIMAG = (ImageCC*)pChunk;
			}
			break;

		case CHUNK_PDAT:
			if (pPDAT == NULL) {
				pPDAT = (PixelChunk*)pChunk;
			}
			break;

		case CHUNK_VDL:
			if (pVDL == NULL) {
				pVDL = (VdlChunk*)pChunk;
			}
			break;

		case CHUNK_CPYR:
		case CHUNK_DESC:
		case CHUNK_KWRD:
		case CHUNK_CRDT:
		case CHUNK_XTRA:
			break;

		default:
			// error
			break;
		}
	}

	// figure out the size of the pixel data
	if (pIMAG == NULL) {
		tempSize = ((240 + 1) >> 1) * 320 * sizeof(uint32);
	} else {
		tempSize = LONG_ENDIAN_FLIP(pIMAG->bytesperrow) * LONG_ENDIAN_FLIP(pIMAG->h);
	}

	if (tempSize > destbufsize) {
		return NULL;
	} else if (tempSize < destbufsize) {
		//VRAMIOReq = CreateVRAMIOReq();
		//+SetVRAMPages(VRAMIOReq, dest, 0, sc->sc_NumBitmapPages, ~0);
		//DeleteVRAMIOReq(VRAMIOReq);
	}

	if (pPDAT != NULL) {
		memcpy(dest, pPDAT->pixels, tempSize);
	} else {
		return NULL;
	}

	correctPixelDataEndianess((uint16*)dest, destbufsize / 2);

	return dest;
}

void *LoadImage(char *filename, ubyte* dest, VdlChunk **rawVDLPtr, ScreenContext *sc)
{
	char* buffer;

	FILE* f = fopen(filename, "rb");
	int size;

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

	buffer = malloc(size);
	if (buffer) {
		fread(buffer, 1, size, f);
		return parseImg(buffer, size, dest, sc);
	} else {
		// something went wrong
		return NULL;
	}
}
