#include "celutils.h"
#include "form3DO.h"

#include "hardware.h"
#include "mem.h"
#include "string.h"

#include <stdio.h>
#include <stdlib.h>

static ubyte bppTable[] = { 0, 1, 2, 4, 6, 8, 16, 0 };

/*----------------------------------------------------------------------------
 * GetCelBitsPerPixel()
 *	Calc and return the bits-per-pixel of a cel.
 *--------------------------------------------------------------------------*/

int32 GetCelBitsPerPixel(CCB* cel)
{
	return bppTable[CEL_PRE0WORD(cel) & PRE0_BPP_MASK];
}

/*----------------------------------------------------------------------------
 * GetCelBytesPerRow()
 *	Calc and return the bytes-per-row of a cel.
 *--------------------------------------------------------------------------*/

int32 GetCelBytesPerRow(CCB* cel)
{
	int32	woffset;
	uint32	pre1;

	pre1 = CEL_PRE1WORD(cel);

	if (GetCelBitsPerPixel(cel) < 8) {
		woffset = (pre1 & PRE1_WOFFSET8_MASK) >> PRE1_WOFFSET8_SHIFT;
	}
	else {
		woffset = (pre1 & PRE1_WOFFSET10_MASK) >> PRE1_WOFFSET10_SHIFT;
	}

	return ((woffset + PRE1_WOFFSET_PREFETCH) * sizeof(int32));

}

/*----------------------------------------------------------------------------
 * GetCelDataBufferSize()
 *	Calc and return the size needed for a cel's data buffer.
 *--------------------------------------------------------------------------*/

int32 GetCelDataBufferSize(CCB* cel)
{
	return (cel->ccb_Height * GetCelBytesPerRow(cel)) + (cel->ccb_Flags & CCB_CCBPRE) ? 0 : 8;
}

int32 InitCel(CCB* cel, int32 w, int32 h, int32 bpp, int32 options)
{
	int32		rowBytes;
	int32		rowWOFFSET;
	int32		wPRE;
	int32		hPRE;

	/*------------------------------------------------------------------------
	 * Zero out the CCB fields that don't need special values.
	 *----------------------------------------------------------------------*/

	cel->ccb_NextPtr = NULL;
	cel->ccb_SourcePtr = NULL;
	cel->ccb_PLUTPtr = NULL;
	cel->ccb_XPos = 0;
	cel->ccb_YPos = 0;
	cel->ccb_HDY = 0;
	cel->ccb_VDX = 0;
	cel->ccb_HDDX = 0;
	cel->ccb_HDDY = 0;

	/*------------------------------------------------------------------------
	 * Set up the CCB fields that need non-zero values.
	 *----------------------------------------------------------------------*/

	cel->ccb_HDX = 1 << 20;
	cel->ccb_VDY = 1 << 16;
	cel->ccb_PIXC = PIXC_OPAQUE;
	cel->ccb_Flags = CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_PPABS |
		CCB_LDSIZE | CCB_LDPRS | CCB_LDPPMP |
		CCB_CCBPRE | CCB_YOXY | CCB_USEAV | CCB_NOBLK |
		CCB_ACE | CCB_ACW | CCB_ACCW |
		((options & INITCEL_CODED) ? CCB_LDPLUT : 0L);

	/*------------------------------------------------------------------------
	 * massage the width/height values.
	 *	we have to set the bytes-per-row value to a a word-aligned value,
	 *	and further have to allow for the cel engine's hardwired minimum
	 *	of two words per row even when the pixels would fit in one word.
	 *----------------------------------------------------------------------*/

	rowBytes = (((w * bpp) + 31) >> 5) << 2;
	if (rowBytes < 8) {
		rowBytes = 8;
	}
	rowWOFFSET = (rowBytes >> 2) - PRE1_WOFFSET_PREFETCH;

	wPRE = (w - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT;
	hPRE = (h - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT;

	cel->ccb_Width = w;
	cel->ccb_Height = h;

	/*------------------------------------------------------------------------
	 * fill in the CCB preamble fields.
	 *----------------------------------------------------------------------*/

	if (!(options & INITCEL_CODED)) {
		hPRE |= PRE0_LINEAR;
	}

	wPRE |= PRE1_TLLSB_PDC0;	/* Use blue LSB from source pixel or PLUT blue LSB. */

	switch (bpp) {
	case 1:
		cel->ccb_PRE0 = hPRE | PRE0_BPP_1;
		cel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET8_SHIFT);
		break;
	case 2:
		cel->ccb_PRE0 = hPRE | PRE0_BPP_2;
		cel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET8_SHIFT);
		break;
	case 4:
		cel->ccb_PRE0 = hPRE | PRE0_BPP_4;
		cel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET8_SHIFT);
		break;
	case 6:
		cel->ccb_PRE0 = hPRE | PRE0_BPP_6;
		cel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET8_SHIFT);
		break;
	case 8:
		cel->ccb_PRE0 = hPRE | PRE0_BPP_8;
		cel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET10_SHIFT);
		break;
	default:
		/* fall thru */
	case 16:
		cel->ccb_PRE0 = hPRE | PRE0_BPP_16;
		cel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET10_SHIFT);
		break;
	}

	return (h * rowBytes);
}

CCB* CreateCel(int32 w, int32 h, int32 bpp, int32 options, void* databuf)
{
	int32 allocExtra;
	int32 bufferBytes;
	CCB* cel = NULL;

	if (bpp < 8) {					/* only 8 and 16 bit cels can be uncoded, for */
		options = CREATECEL_CODED;	/* anything less, force coded flag on. */
	}

	allocExtra = (options & CREATECEL_CODED) ? sizeof(uint16[32]) : 0L;

	cel = AllocMem(sizeof(CCB) + allocExtra, MEMTYPE_ANY);

	bufferBytes = InitCel(cel, w, h, bpp, options);

	cel->ccb_PLUTPtr = (options & CREATECEL_CODED) ? (void*)(cel + 1) : NULL;

	if (databuf == NULL) {
		databuf = AllocMem(bufferBytes, MEMTYPE_ANY);
	}
	cel->ccb_SourcePtr = (CelData*)databuf;

	return cel;
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

static void correctCelDataEndianess16bpp(uint16 *data, int pixelCount)
{
	if (!data || pixelCount <= 0) return;

	while(pixelCount-- > 0) {
		const uint16 c = *data;
		*data++ = SHORT_ENDIAN_FLIP(c);
	}
}

static void correctCelEndianess(CCB *cel)
{
	static int bppTable[8] = {0, 1,2,4,6,8,16, 0};

	uint32* start = &cel->ccb_XPos;
	uint32* end = &cel->ccb_Height;
	uint32* ptr;
	int bpp;

	for (ptr = start; ptr <= end; ptr++) {
		const uint32 value = LONG_ENDIAN_FLIP(*ptr);
		*ptr = value;
	}

	bpp = bppTable[cel->ccb_PRE0 & PRE0_BPP_MASK];
	if (bpp > 8) {
		if (!(cel->ccb_Flags & CCB_PACKED)) {
			correctCelDataEndianess16bpp((uint16*)cel->ccb_SourcePtr, cel->ccb_Width * cel->ccb_Height);
		}
		return;
	}

	if (!(cel->ccb_PRE0 & PRE0_LINEAR)) {
		if (bpp == 8) {
			bpp = 5;
		}
		correctPalEndianess(cel->ccb_PLUTPtr, 1 << bpp);
	}
}

CCB* CloneCel(CCB* src, int32 options)
{
	int32	allocExtra;
	void* dataBuf;
	CCB* cel = NULL;

	allocExtra = ((options & CLONECEL_COPY_PLUT) ? sizeof(uint16[32]) : 0L);

	cel = AllocMem(sizeof(CCB) + allocExtra, MEMTYPE_ANY);

	memcpy(cel, src, sizeof(CCB));
	cel->ccb_Flags |= CCB_LAST;
	cel->ccb_NextPtr = NULL;

	if ((options & CLONECEL_COPY_PLUT) && src->ccb_PLUTPtr != NULL) {
		cel->ccb_PLUTPtr = cel + 1;
		memcpy(cel->ccb_PLUTPtr, src->ccb_PLUTPtr, sizeof(uint16[32]));
	}

	if (options & CLONECEL_COPY_PIXELS) {
		allocExtra = GetCelDataBufferSize(src);
		if ((dataBuf = AllocMem(allocExtra, MEMTYPE_TRACKSIZE | MEMTYPE_CEL)) == NULL) {
			//DIAGNOSE_SYSERR(NOMEM, ("Can't allocate cel data buffer\n"));
			goto ERROR_EXIT;
		}
		//ModifyMagicCel_(cel, DELETECELMAGIC_CCB_AND_DATA, dataBuf, NULL);
		cel->ccb_SourcePtr = (CelData*)dataBuf;
		memcpy(cel->ccb_SourcePtr, src->ccb_SourcePtr, allocExtra);
	}

	return cel;

ERROR_EXIT:

	return DeleteCel(cel);
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

#define DUMMY_BUFFER	((void *)1)	/* a dummy non-NULL data buffer pointer */

#define	PIXCL_BLEND_8	(PPMPC_1S_CFBD | PPMPC_SF_8 | PPMPC_2S_PDC)

CCB* CreateBackdropCel(int32 width, int32 height, int32 color, int32 opacityPct)
{
	static Point quad[4];

	CCB* pCel;
	int32	scaleMul;
	int32	r, g, b;
	int32	scaledColor;
	//SRect	srect;

	/*------------------------------------------------------------------------
	 * create a 16-bit uncoded cel at a 1x1 source data size, then map its
	 * projection to the caller-specified width and height.
	 *----------------------------------------------------------------------*/

	if ((pCel = CreateCel(1, 1, 16, CREATECEL_UNCODED, DUMMY_BUFFER)) == NULL) {
		return NULL;	/* error already reported by CreateCel(). */
	}

	//MapCelToSRect(pCel, SRectFromIVal(&srect, 0, 0, width, height));
	quad[0].pt_X = 0; quad[0].pt_Y = 0;
	quad[1].pt_X = width; quad[1].pt_Y = 0;
	quad[2].pt_X = width; quad[2].pt_Y = height;
	quad[3].pt_X = 0; quad[3].pt_Y = height;
	MapCel(pCel, quad);

	/*------------------------------------------------------------------------
	 * Set up the PIXC word so that the primary source is the current frame
	 * buffer pixel, scaled according to the opacity percent, and the
	 * secondary source is a single pixel of the requested color, pre-scaled
	 * to the inverse of the primary source scaling factor.
	 *----------------------------------------------------------------------*/

	if (opacityPct == 0) {						/* 0% opacity means this is a  */
		scaledColor = 0;						/* 'virtual' cel that doesn't */
		pCel->ccb_Flags |= CCB_SKIP;			/* display anything. */
	}
	else {
		pCel->ccb_Flags |= CCB_BGND;			/* don't skip 0-valued pixels, */
		pCel->ccb_PRE0 |= PRE0_BGND;			/* really, trust me, don't skip them. */

		color &= 0x00007FFF;

		scaleMul = (opacityPct + 12) / 13;		/* put 1-100% in range of 1-8 multiplier */

		if (scaleMul == 8) {
			scaledColor = color;
			pCel->ccb_PIXC = PIXC_OPAQUE; 			/* opaque PIXC for 100% opacity */
		}
		else {
			r = (color >> 10) & 0x1F;				/* isolate the color components. */
			g = (color >> 5) & 0x1F;
			b = (color >> 0) & 0x1F;

			r = (r * scaleMul) / 8;					/* scale color components to the inverse */
			g = (g * scaleMul) / 8;					/* of the scaling used for the current */
			b = (b * scaleMul) / 8;					/* frame buffer pixel. */

			scaledColor = (r << 10) | (g << 5) | (b << 0);	/* reassemble color components. */

			scaleMul = 8 - scaleMul;				/* inverse multiplier for CFDB scaling */

			pCel->ccb_PIXC = PIXCL_BLEND_8 | ((scaleMul - 1) << PPMPC_MF_SHIFT);
		}
	}

	/*------------------------------------------------------------------------
	 * Store the pre-scaled color pixel in the PLUT pointer in the CCB, and
	 * point the source data pointer to it.  This just saves memory; uncoded
	 * cels don't have a PLUT, so we use the PLUTPtr field as a little 4-byte
	 * pixel data buffer.  It doesn't matter that the pixel doesn't even
	 * slightly resemble a pointer, because the LDPLUT flag isn't set and
	 * thus the cel engine will never read the PLUTPtr field.  (Sneaky, huh?)
	 *----------------------------------------------------------------------*/

	#ifdef BIG_ENDIAN
		pCel->ccb_PLUTPtr = (void*)((scaledColor << 16));
	#else
		pCel->ccb_PLUTPtr = (void*)scaledColor;
	#endif
	pCel->ccb_SourcePtr = (CelData*)&pCel->ccb_PLUTPtr;

	return pCel;

}
