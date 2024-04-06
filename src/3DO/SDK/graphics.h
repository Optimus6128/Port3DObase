#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"
#include "sintab.h"
#include "hardware.h"

#define SPORTCMD_CLONE  4
#define SPORTCMD_COPY   5
#define FLASHWRITE_CMD  6

#define DI_TYPE_DEFAULT	0
/* Standard 320x240 NTSC display */
#define DI_TYPE_NTSC	1
/* Narrow PAL display (320x288) */
#define DI_TYPE_PAL1	2
/* Normal PAL display (384x288) */
#define DI_TYPE_PAL2	3

#define MakeRGB15(r,g,b) (uint16)(((uint32)(r)<<10)|((uint32)(g)<<5)|(uint32)(b))

typedef uint32 CelData[];
typedef int32  Coord;

typedef struct Point
{
	Coord pt_X;
	Coord pt_Y;
} Point;

typedef struct CCB
{
	uint32 ccb_Flags;

	struct CCB* ccb_NextPtr;
	CelData* ccb_SourcePtr;
	void* ccb_PLUTPtr;

	Coord ccb_XPos;
	Coord ccb_YPos;
	int32  ccb_HDX;
	int32  ccb_HDY;
	int32  ccb_VDX;
	int32  ccb_VDY;
	int32  ccb_HDDX;
	int32  ccb_HDDY;
	uint32 ccb_PIXC;
	uint32 ccb_PRE0;
	uint32 ccb_PRE1;

	/* These are special fields, tacked on to support some of the
	 * rendering functions.
	 */
	int32  ccb_Width;
	int32  ccb_Height;
} CCB;

typedef struct Bitmap {
	//ItemNode bm;

	ubyte* bm_Buffer;

	int32 bm_Width;
	int32 bm_Height;
	int32 bm_VerticalOffset;
	int32 bm_Flags;

	int32 bm_ClipWidth;
	int32 bm_ClipHeight;
	int32 bm_ClipX;
	int32 bm_ClipY;
	int32 bm_WatchDogCtr;  /* JCR */
	int32 bm_SysMalloc;  /* If set, CreateScreenGroup MALLOCED for bm. JCR */

	/* List of tasks that have share access to this Bitmap */
	//List bm_SharedList;

	int32 bm_CEControl;
	int32 bm_REGCTL0;
	int32 bm_REGCTL1;
	int32 bm_REGCTL2;
	int32 bm_REGCTL3;
} Bitmap;

typedef struct VDLmapColor
{
	uint8 r, g, b;
}VDLmapColor;

// Hacky externs for main.c to render screen or alter input events
extern uint16* vram3DOptr;
extern VDLmapColor VDLmap[32];

void DisplayScreen(Item screenItem0, Item screenItem1);
void SetScreenColors(Item screenItem, uint32* entries, int32 count);

void MapCel(CCB* ccb, Point* quad);
Err DrawCels(Item bitmapItem, CCB* ccb);

#define MakeCLUTColorEntry(index,r,g,b) ((((uint32)(index)<<24)|VDL_FULLRGB|((uint32)(r)<<16)|((uint32)(g)<<8)|((uint32)(b))))
void SetScreenColors(Item screenItem, uint32 *entries, int32 count);

void OpenGraphicsFolio();

void SetCEControl(Item bitmapItem, int32 controlWord, int32 controlMask);
void EnableHAVG(Item screenItem);
void EnableVAVG(Item screenItem);

Item CreateVRAMIOReq();
Item GetVBLIOReq();
void WaitVBL (Item ioreq, uint32 numfields);

void DeleteItem(Item i);

#define	DeleteVRAMIOReq(x)	DeleteItem(x)
#define	DeleteVBLIOReq(x)	DeleteItem(x)

#endif
