#ifndef CELUTILS_H
#define CELUTILS_H

#include "types.h"
#include "graphics.h"

#define CLONECEL_CCB_ONLY		0x00000000
#define CLONECEL_COPY_PIXELS	0x00000001
#define CLONECEL_COPY_PLUT		0x00000002

#define INITCEL_UNCODED			0x00000000
#define INITCEL_CODED			0x00000001

#define CREATECEL_UNCODED		INITCEL_UNCODED
#define CREATECEL_CODED			INITCEL_CODED

CCB* CreateCel(int32 width, int32 height, int32 bitsPerPixel, int32 options, void* dataBuf);
CCB* LoadCel(char* filename, uint32 memTypeBits);
CCB* ParseCel(void* buffer, int32 size);
void UnloadCel(CCB* cel);
CCB* DeleteCel(CCB* cel);

void LinkCel(CCB* ccb, CCB* nextCCB);

#endif
