#include "celutils.h"
#include "form3DO.h"

#include "hardware.h"

#include <stdio.h>
#include <stdlib.h>




CCB* CreateCel(int32 width, int32 height, int32 bitsPerPixel, int32 options, void* dataBuf)
{
	// Dummy (We use createCel with lowercase, our own. But would be good to provide the standard option for other people's projects)

	return NULL;
}

static CCB* parseCel(void* buffer, int32 size)
{
	CCB* pCCB = NULL;
	CCB* firstCCB = NULL;
	CCB* lastCCB = NULL;
	int32* pPLUT = NULL;
	CelData* pPDAT = NULL;

	int numCCBs = 0;
	int numPLUTs = 0;

	char* tempBuf = (char*)buffer;
	int32 tempSize = size;

	char* pChunk;
	uint32 chunk_ID;

	while ((pChunk = GetChunk(&chunk_ID, &tempBuf, &tempSize)) != NULL) {
		switch (chunk_ID) {
		case CHUNK_CCB:
			++numCCBs;
			pCCB = (CCB*)&((CCC*)pChunk)->ccb_Flags;
			pCCB->ccb_NextPtr = NULL;
			pCCB->ccb_SourcePtr = NULL;
			pCCB->ccb_PLUTPtr = NULL;

			#ifndef BIG_ENDIAN
				pCCB->ccb_Flags = LONG_ENDIAN_FLIP(pCCB->ccb_Flags);
			#endif

			pCCB->ccb_Flags |= CCB_SPABS
				| CCB_PPABS
				| CCB_NPABS
				| CCB_YOXY
				| CCB_LAST;  /* V32 anims might not have these set */

			if (firstCCB == NULL) {
				firstCCB = pCCB;
			}

			if (lastCCB != NULL) {
				if (lastCCB->ccb_SourcePtr == NULL) {
					printf("Found a new CCB without finding a PDAT for the prior CCB\n");
					goto ERROR_EXIT;
				}
				lastCCB->ccb_Flags &= ~CCB_LAST;
				lastCCB->ccb_NextPtr = pCCB;
			}
			lastCCB = pCCB;
			break;

		case CHUNK_PDAT:
			pPDAT = (CelData*)((PixelChunk*)pChunk)->pixels;
			if (lastCCB == NULL) {
				printf("Found a PDAT before finding any CCB\n");
				goto ERROR_EXIT;
			}
			else if (lastCCB->ccb_SourcePtr != NULL) {
				printf("Found two PDATs without an intervening CCB\n");
				goto ERROR_EXIT;
			}
			lastCCB->ccb_SourcePtr = pPDAT;
			break;

		case CHUNK_PLUT:
			++numPLUTs;
			pPLUT = (int32*)((PLUTChunk*)pChunk)->PLUT;
			if (lastCCB == NULL) {
				printf("Found a PLUT before finding any CCB\n");
				goto ERROR_EXIT;
			}
			else if (lastCCB->ccb_PLUTPtr != NULL) {
				printf("Found two PLUTs without an intervening CCB\n");
				goto ERROR_EXIT;
			}
			lastCCB->ccb_PLUTPtr = pPLUT;
			break;

		case CHUNK_CPYR:
		case CHUNK_DESC:
		case CHUNK_KWRD:
		case CHUNK_CRDT:
		case CHUNK_XTRA:
			break;

		default:
			printf("Unexpected chunk ID %.4s, ignored\n", pChunk);
			break;
		}
	}

	/*------------------------------------------------------------------------
	 * do a couple basic sanity checks
	 *----------------------------------------------------------------------*/

	if (lastCCB == NULL) {
		printf("No CCB found in buffer\n");
		goto ERROR_EXIT;
	}

	if (lastCCB->ccb_SourcePtr == NULL) {
		printf("Found CCB without associated PDAT in buffer\n");
		goto ERROR_EXIT;
	}

	return firstCCB;

ERROR_EXIT:

	return NULL;
}

static void correctPalEndianess(uint16 *pal, int numColors)
{
	if (!pal || numColors <= 0 || numColors > 32) return;

	while(numColors-- > 0) {
		const uint16 c = *pal;
		*pal++ = SHORT_ENDIAN_FLIP(c);
	}
}

static void correctCelEndianess(CCB *cel)
{
	uint32* start = &cel->ccb_XPos;
	uint32* end = &cel->ccb_Height;
	uint32* ptr;

	for (ptr = start; ptr <= end; ptr++) {
		const uint32 value = LONG_ENDIAN_FLIP(*ptr);
		*ptr = value;
	}

	if (!(cel->ccb_PRE0 & PRE0_LINEAR)) {
		static int bppTable[8] = {0, 1,2,4,6,8,16, 0};
		int bpp = bppTable[cel->ccb_PRE0 & PRE0_BPP_MASK];

		if (bpp > 8) return;
		if (bpp == 8) {
			bpp = 5;
		}
		correctPalEndianess(cel->ccb_PLUTPtr, 1 << bpp);
	}
}

CCB* LoadCel(char* filename, uint32 memTypeBits)
{
	CCB* cel = NULL;
	char* buffer;

	FILE* f = fopen(filename, "rb");
	int size;

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

	buffer = malloc(size);
	if (buffer) {
		fread(buffer, 1, size, f);
	} else {
		// something went wrong
		return NULL;
	}

	cel = parseCel(buffer, size);

#ifndef BIG_ENDIAN
	correctCelEndianess(cel);
#endif

	return cel;
}

void UnloadCel(CCB* cel)
{
	// TODO
}
CCB* DeleteCel(CCB* cel)
{
	return 0;
	// Dummy
}

void LinkCel(CCB* ccb, CCB* nextCCB)
{
	if (!ccb || !nextCCB) return;

	ccb->ccb_NextPtr = nextCCB;
	ccb->ccb_Flags &= ~CCB_LAST;
}
