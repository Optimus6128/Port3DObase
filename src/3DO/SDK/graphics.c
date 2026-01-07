#include "graphics.h"

#include <SDL2/SDL.h>

#include "../main.h"
#include "hardware/CelRenderer.h"

#include <string.h>
#include <stdlib.h>

static VDLmapColor VDLmap[32];

void initVDLmap()
{
	int i;
	for (i = 0; i < 32; ++i) {
		VDLmap[i].r = i << 3;
		VDLmap[i].g = i << 3;
		VDLmap[i].b = i << 3;
	}
}

void DisplayScreen(Item screenItem0, Item screenItem1)
{
	Bitmap* bitmap = (Bitmap*)screenItem0;

	update3DOscreenSDL((uint16*)bitmap->bm_Buffer, VDLmap);
}

void SetScreenColors(Item screenItem, uint32 *entries, int32 count)
{
	int i;
	for (i=0; i<count; ++i) {
		uint32 c = *entries++;
		int index = (c >> 24) & 31;
		VDLmap[index].r = (c >> 16) & 255;
		VDLmap[index].g = (c >> 8) & 255;
		VDLmap[index].b = c & 255;
	}
}

void MapCel(CCB* ccb, Point* quad)
{
	ccb->ccb_XPos = ((quad[0].pt_X << 16) & 0xffff0000) + 0x8000;
	ccb->ccb_YPos = ((quad[0].pt_Y << 16) & 0xffff0000) + 0x8000;
	ccb->ccb_HDX = ((quad[1].pt_X - quad[0].pt_X) << 20) / ccb->ccb_Width;
	ccb->ccb_HDY = ((quad[1].pt_Y - quad[0].pt_Y) << 20) / ccb->ccb_Width;
	ccb->ccb_VDX = ((quad[3].pt_X - quad[0].pt_X) << 16) / ccb->ccb_Height;
	ccb->ccb_VDY = ((quad[3].pt_Y - quad[0].pt_Y) << 16) / ccb->ccb_Height;
	ccb->ccb_HDDX = ((quad[2].pt_X - quad[3].pt_X - quad[1].pt_X + quad[0].pt_X)
		<< 20) / (ccb->ccb_Width * ccb->ccb_Height);
	ccb->ccb_HDDY = ((quad[2].pt_Y - quad[3].pt_Y - quad[1].pt_Y + quad[0].pt_Y)
		<< 20) / (ccb->ccb_Width * ccb->ccb_Height);
}

static void prepareCel(CCB *src, CCB *dst)
{
	static CCB prevCelState;

	const uint32 srcFlags = src->ccb_Flags;
	int32* celDataPtr = &src->ccb_XPos;

	dst->ccb_Flags = srcFlags;
	dst->ccb_NextPtr = src->ccb_NextPtr;
	dst->ccb_SourcePtr = src->ccb_SourcePtr;

	if (srcFlags & CCB_LDPLUT) {
		dst->ccb_PLUTPtr = src->ccb_PLUTPtr;
		prevCelState.ccb_PLUTPtr = src->ccb_PLUTPtr;
	} else {
		dst->ccb_PLUTPtr = prevCelState.ccb_PLUTPtr;
	}

	if (srcFlags & CCB_YOXY) {
		memcpy(&dst->ccb_XPos, celDataPtr, 8);
		memcpy(&prevCelState.ccb_XPos, celDataPtr, 8);
		celDataPtr += 2;
	} else {
		memcpy(&dst->ccb_XPos, &prevCelState.ccb_XPos, 8);
	}

	if (srcFlags & CCB_LDSIZE) {
		memcpy(&dst->ccb_HDX, celDataPtr, 16);
		memcpy(&prevCelState.ccb_HDX, celDataPtr, 16);
		celDataPtr += 4;
	} else {
		memcpy(&dst->ccb_HDX, &prevCelState.ccb_HDX, 16);
	}

	if (srcFlags & CCB_LDPRS) {
		memcpy(&dst->ccb_HDDX, celDataPtr, 8);
		memcpy(&prevCelState.ccb_HDDX, celDataPtr, 8);
		celDataPtr += 2;
	} else {
		memcpy(&dst->ccb_HDDX, &prevCelState.ccb_HDDX, 8);
	}

	if (srcFlags & CCB_LDPPMP) {
		dst->ccb_PIXC = *celDataPtr++;
		prevCelState.ccb_PIXC = dst->ccb_PIXC;
	} else {
		dst->ccb_PIXC = prevCelState.ccb_PIXC;
	}

	if (srcFlags & CCB_CCBPRE) {
		memcpy(&dst->ccb_PRE0, celDataPtr, 8);
	} else {
		memcpy(&dst->ccb_PRE0, dst->ccb_SourcePtr, 8);
		dst->ccb_SourcePtr = (CelData*)((uint32*)dst->ccb_SourcePtr + 2);
	}
}

Err DrawCels(Item bitmapItem, CCB* ccb)
{
	static CCB preparedCel;

	Bitmap* bitmap = (Bitmap*)bitmapItem;
	uint16* dst = (uint16*)bitmap->bm_Buffer;

	while(ccb != NULL) {
		if (!(ccb->ccb_Flags & CCB_SKIP)) {
			prepareCel(ccb, &preparedCel);
			renderCel(&preparedCel, dst);
		}
		if (ccb->ccb_Flags & CCB_LAST) break;
		ccb = ccb->ccb_NextPtr;
	};

	return 0;
	// TODO
}

void OpenGraphicsFolio()
{
	// Dummy
}

void SetCEControl(Item bitmapItem, int32 controlWord, int32 controlMask)
{
	// Dummy
}

void EnableHAVG(Item screenItem)
{
	// Dummy
}

void EnableVAVG(Item screenItem)
{
	// Dummy
}

Item CreateVRAMIOReq()
{
	// Dummy
	return 0;
}

Item GetVBLIOReq()
{
	// Dummy
	return 0;
}

void WaitVBL (Item ioreq, uint32 numfields)
{
	static int timeTicks = 0;

	do {} while (SDL_GetTicks() - timeTicks < 20);
	timeTicks = SDL_GetTicks();
}

void DeleteItem(Item i)
{
	// Dummy
}
