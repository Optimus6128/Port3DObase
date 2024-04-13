#ifndef MAIN_H
#define MAIN_H

#define SCREEN_WIDTH_3DO 320
#define SCREEN_HEIGHT_3DO 240

#define SCREEN_SCALE 4
#define SDL_SCREEN_WIDTH (SCREEN_WIDTH_3DO * SCREEN_SCALE)
#define SDL_SCREEN_HEIGHT (SCREEN_HEIGHT_3DO * SCREEN_SCALE)

#include "hardware.h"
#include "event.h"
#include "graphics.h"
#include "types.h"

typedef struct ScreenBuffer
{
	int width, height;
	unsigned int *vram;
}ScreenBuffer;

void updateInputSDL(ControlPadEventData *controlPadEventData3DO);
void update3DOscreenSDL(uint16 *vram3DOptr, VDLmapColor *VDLmap);

#endif
