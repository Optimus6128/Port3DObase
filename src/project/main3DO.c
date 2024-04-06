#include "main3DO.h"
#include "types.h"

static ScreenContext screen;

static Item bitmapItems[SCREEN_PAGES];
static Bitmap *bitmaps[SCREEN_PAGES];

static int visibleScreenPage = 0;

static void initGraphics()
{
	int i;
	static bool hasInitGraphics = false;

	if (!hasInitGraphics) {
		CreateBasicDisplay(&screen, DI_TYPE_DEFAULT, SCREEN_PAGES);
		for (i = 0; i < SCREEN_PAGES; i++)
		{
			bitmapItems[i] = screen.sc_BitmapItems[i];
			bitmaps[i] = screen.sc_Bitmaps[i];
		}
		hasInitGraphics = true;
	}
}

static void clearScreen(ubyte test)
{
    Bitmap *screenBmp = bitmaps[visibleScreenPage];
    memset((unsigned char*)screenBmp->bm_Buffer, test, screenBmp->bm_Width * screenBmp->bm_Height * 2);
}

static void display()
{
    DisplayScreen(screen.sc_Screens[visibleScreenPage], 0 );

    visibleScreenPage = (visibleScreenPage + 1) % SCREEN_PAGES;
}


int main3DO()
{
	initGraphics();

	//while(true) {
		clearScreen(rand());
		display();
	//}
	return 0;
}
