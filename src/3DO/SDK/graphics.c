#include "graphics.h"

#include <SDL2/SDL.h>

#include "../main.h"
#include "hardware/CelRenderer.h"

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

Err DrawCels(Item bitmapItem, CCB* ccb)
{
	Bitmap* bitmap = (Bitmap*)bitmapItem;
	uint16* dst = (uint16*)bitmap->bm_Buffer;

	while(ccb != NULL) {
		if (!(ccb->ccb_Flags & CCB_SKIP)) {
			renderCel(ccb, dst);
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
