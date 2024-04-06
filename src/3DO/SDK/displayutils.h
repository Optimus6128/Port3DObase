#ifndef DISPLAYUTILS_H
#define DISPLAYUTILS_H

#include "types.h"
#include "graphics.h"

#include "form3DO.h"



/* anybody ever hear of hexa-buffering? */
#define MAXSCREENS 6

/* briefly describes a display */
typedef struct ScreenContext
{
	Item        sc_ScreenGroup;                /* associated screen group    */
	uint32      sc_DisplayType;                /* DI_TYPE_* from graphics.h  */

	uint32      sc_NumScreens;                 /* # of screens created       */
	uint32      sc_CurrentScreen;              /* displayed screen           */
	Item        sc_ScreenItems[MAXSCREENS];    /* item for the screen        */
	Item        sc_BitmapItems[MAXSCREENS];    /* bitmap item for the screen */
	Bitmap*		sc_Bitmaps[MAXSCREENS];        /* structure itself           */

	uint32      sc_NumBitmapPages;      /* # pages of memory for each bitmap */
	uint32      sc_NumBitmapBytes;      /* # bytes of memory for each bitmap */
	uint32      sc_BitmapBank;          /* bank of memory for all bitmaps    */
	uint32      sc_BitmapWidth;         /* pixel width of each bitmap        */
	uint32      sc_BitmapHeight;        /* pixel height of each bitmap       */
} ScreenContext;

/* remap old names to new ones */
#define sc_nScreens          sc_NumScreens
#define sc_curScreen         sc_CurrentScreen
#define sc_Screens           sc_ScreenItems
#define sc_nFrameBufferPages sc_NumBitmapPages
#define sc_nFrameByteCount   sc_NumBitmapBytes

Item CreateBasicDisplay(ScreenContext* sc, uint32 displayType, uint32 numScreens);
void DeleteBasicDisplay(ScreenContext* sc);

void* LoadImage(char* filename, ubyte* dest, VdlChunk** rawVDLPtr, ScreenContext* sc);

#endif
