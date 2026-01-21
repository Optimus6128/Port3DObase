#ifndef CEL_RENDERER_H
#define CEL_RENDERER_H

#include "types.h"
#include "hardware.h"
#include "graphics.h"

// For the time being, I set the default screen width/height, but that could change if PAL or screen region
// I will have to change some functions to get it from the screenItem or something..
#define SCREEN_W 320
#define SCREEN_H 240

#define GET_CEL_WIDTH(cel) ((cel->ccb_PRE1 & PRE1_TLHPCNT_MASK) + PRE1_TLHPCNT_PREFETCH)
#define GET_CEL_HEIGHT(cel) (((cel->ccb_PRE0 & PRE0_VCNT_MASK) >> PRE0_VCNT_SHIFT) + PRE0_VCNT_PREFETCH)

#define VRAM_OFS(x,y) ((y) >> 1) * SCREEN_W * 2 + ((y) & 1) + ((x) << 1)

void initCelRenderer();
void renderCel(CCB* cel, uint16* dst);

#endif
