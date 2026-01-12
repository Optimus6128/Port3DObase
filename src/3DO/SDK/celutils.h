#ifndef CELUTILS_H
#define CELUTILS_H

#include "types.h"
#include "graphics.h"
#include "operamath.h"

#ifndef AddToPtr
#define AddToPtr(ptr, val) ((void*)((((char *)(ptr)) + (long)(val))))
#endif

#define CLONECEL_CCB_ONLY		0x00000000
#define CLONECEL_COPY_PIXELS	0x00000001
#define CLONECEL_COPY_PLUT		0x00000002

#define INITCEL_UNCODED			0x00000000
#define INITCEL_CODED			0x00000001

#define CREATECEL_UNCODED		INITCEL_UNCODED
#define CREATECEL_CODED			INITCEL_CODED

#define IS_LASTCEL(ccb)		(((ccb)->ccb_Flags & CCB_LAST) || (ccb)->ccb_NextPtr == NULL)

#define CEL_NEXTPTR(ccb)	(CCB *)		(((ccb)->ccb_Flags & CCB_LAST)  ? NULL : ((ccb)->ccb_Flags & CCB_NPABS) ? (ccb)->ccb_NextPtr : AddToPtr(&((ccb)->ccb_NextPtr), (int32)((ccb)->ccb_NextPtr)+4))
#define CEL_PLUTPTR(ccb)	(uint16 *)	(((ccb)->ccb_Flags & CCB_PPABS) ? (ccb)->ccb_PLUTPtr   : AddToPtr(&((ccb)->ccb_PLUTPtr), (int32)((ccb)->ccb_PLUTPtr)+4))
#define CEL_DATAPTR(ccb)	(CelData *)	(((ccb)->ccb_Flags & CCB_SPABS) ? (ccb)->ccb_SourcePtr : AddToPtr(&((ccb)->ccb_SourcePtr), (int32)((ccb)->ccb_SourcePtr)+4))

#define CEL_PRE0WORD(ccb)	(((ccb)->ccb_Flags & CCB_CCBPRE) ? (ccb)->ccb_PRE0 : ((uint32 *)CEL_DATAPTR(ccb))[0])
#define CEL_PRE1WORD(ccb)	(((ccb)->ccb_Flags & CCB_CCBPRE) ? (ccb)->ccb_PRE1 : ((uint32 *)CEL_DATAPTR(ccb))[1])

#define	PIXC_OPAQUE				0x1F001F00


/*----------------------------------------------------------------------------
 * Datatypes.
 *--------------------------------------------------------------------------*/

typedef struct FPoint {	/* Frac16 point */
	frac16	x;
	frac16	y;
} FPoint;

typedef struct IPoint {	/* Integer point */
	int32	x;
	int32	y;
} IPoint;

typedef struct SRect {	/* Size Rectangle	- specifies topleft corner and size */
	IPoint	pos;		/*	x/y of TopLeft corner */
	IPoint	size;		/*	x/y sizes (ie, width and height) */
} SRect;

typedef struct CRect {	/* Corner Rectangle	- specifies diagonal corners */
	IPoint	tl;			/* 	TopLeft corner */
	IPoint	br;			/* 	BottomRight corner */
} CRect;

typedef struct CQuad {	/* Corner quad		- specifies all four corners */
	IPoint	tl;			/*	TopLeft corner */
	IPoint	tr;			/* 	TopRight corner */
	IPoint	br;			/* 	BottomRight corner */
	IPoint	bl;			/* 	BottomLeft corner */
} CQuad;


CCB* CreateCel(int32 width, int32 height, int32 bitsPerPixel, int32 options, void* dataBuf);
int32 InitCel(CCB* cel, int32 w, int32 h, int32 bpp, int32 options);
CCB* CloneCel(CCB* src, int32 options);
CCB* LoadCel(char* filename, uint32 memTypeBits);
CCB* ParseCel(void* buffer, int32 size);
void UnloadCel(CCB* cel);
CCB* DeleteCel(CCB* cel);

void LinkCel(CCB* ccb, CCB* nextCCB);

CCB* CreateBackdropCel(int32 width, int32 height, int32 color, int32 opacityPercent);

int32 GetCelBitsPerPixel(CCB* cel);
int32 GetCelBytesPerRow(CCB* cel);
int32 GetCelDataBufferSize(CCB* cel);

#endif
