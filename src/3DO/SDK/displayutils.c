#include "displayutils.h"
#include "graphics.h"

#include <stdlib.h>


static void initVDLmap()
{
	int i;
	for (i = 0; i < 32; ++i) {
		VDLmap[i].r = i << 3;
		VDLmap[i].g = i << 3;
		VDLmap[i].b = i << 3;
	}
}

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

void *LoadImage(char *filename, ubyte* dest, VdlChunk **rawVDLPtr, ScreenContext *sc)
{
	// Dummy
	return NULL;
}
