#include <stdio.h>

#include "TinyFPS.h"

#define FPS_FONT_WIDTH 4
#define FPS_FONT_HEIGHT 5
#define FPS_FONTS_NUM 10
#define FPS_FONT_NUM_PIXELS (FPS_FONTS_NUM * FPS_FONT_WIDTH * FPS_FONT_HEIGHT)

/*
1110 0010 1110 1110 1010 1110 1110 1110 1110 1110
1010 0010 0010 0010 1010 1000 1000 0010 1010 1010
1010 0010 1110 0110 1110 1110 1110 0010 1110 1110
1010 0010 1000 0010 0010 0010 1010 0010 1010 0010
1110 0010 1110 1110 0010 1110 1110 0010 1110 1110
*/

static const unsigned char miniDecimalData[] = {	0xE2, 0xEE, 0xAE, 0xEE, 0xEE,
													0xA2, 0x22, 0xA8, 0x82, 0xAA,
													0xA2, 0xE6, 0xEE, 0xE2, 0xEE,
													0xA2, 0x82, 0x22, 0xA2, 0xA2,
													0xE2, 0xEE, 0x2E, 0xE2, 0xEE };


unsigned int miniDecimalFonts[FPS_FONT_NUM_PIXELS];

static unsigned long startingFpsTime = 0;
static int nFrames = 0;

void initFpsFonts()
{
	unsigned char miniDecimalPixels[FPS_FONT_NUM_PIXELS];

	int i, j;
	int x, y, n;

	int k = 0;
	for (i = 0; i<FPS_FONT_NUM_PIXELS / 8; i++) {
		unsigned char  d = miniDecimalData[i];
		for (j = 0; j<8; j++) {
			unsigned char c = (d & 0x80) >> 7;
			miniDecimalPixels[k++] = c;
			d <<= 1;
		}
	}

	i = 0;
	for (n = 0; n<FPS_FONTS_NUM; n++) {
		for (y = 0; y<FPS_FONT_HEIGHT; y++) {
			for (x = 0; x<FPS_FONT_WIDTH; x++) {
				miniDecimalFonts[i++] = miniDecimalPixels[n*FPS_FONT_WIDTH + x + y * FPS_FONT_WIDTH * FPS_FONTS_NUM] * 0xFFFFFFFF;
			}
		}
	}
}

static void drawFont(int decimal, int posX, int posY, int zoom, ScreenBuffer *screen)
{
	unsigned int *fontData = (unsigned int*)&miniDecimalFonts[decimal * FPS_FONT_WIDTH * FPS_FONT_HEIGHT];
	unsigned int *vram = screen->vram + posY * screen->width + posX;

	int x, y, j, k;

	if (zoom < 1) zoom = 1;
	if (zoom > 16) zoom = 16;

	for (y = 0; y<FPS_FONT_HEIGHT; y++) {
		for (x = 0; x<FPS_FONT_WIDTH; x++) {
			unsigned int c = *fontData++;
			if (c != 0) {
				for (j = 0; j<zoom; j++) {
					for (k = 0; k<zoom; k++) {
						*(vram + j * screen->width + k) = c;
					}
				}
			}
			vram += zoom;
		}
		vram += (-FPS_FONT_WIDTH * zoom + screen->width * zoom);
	}
}

static void drawDecimal(unsigned int number, int posX, int posY, int zoom, ScreenBuffer *screen)
{
	char buffer[8];
	int i = 0;

	sprintf(buffer, "%d", number);

	while (i < 8 && buffer[i] != 0) {
		drawFont(buffer[i] - 48, posX + i * zoom * FPS_FONT_WIDTH, posY, zoom, screen);
		i++;
	}
}

void drawFps(ScreenBuffer *screen, int ticks, int posX, int posY, int zoom)
{
	unsigned long dt = ticks - startingFpsTime;
	static int fps = 0;

	nFrames++;
	if (dt >= 1000)
	{
		fps = (nFrames * 1000) / dt;
		//printf("%d\n", fps);
		startingFpsTime = ticks;
		nFrames = 0;
	}

	drawDecimal(fps, posX, posY, zoom, screen);
}
