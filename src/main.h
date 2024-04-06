#ifndef MAIN_H
#define MAIN_H

#define SCREEN_WIDTH_3DO 320
#define SCREEN_HEIGHT_3DO 240

#define SCREEN_SCALE 4
#define SDL_SCREEN_WIDTH (SCREEN_WIDTH_3DO * SCREEN_SCALE)
#define SDL_SCREEN_HEIGHT (SCREEN_HEIGHT_3DO * SCREEN_SCALE)

typedef struct ScreenBuffer
{
	int width, height;
	unsigned int *vram;
}ScreenBuffer;

#endif
