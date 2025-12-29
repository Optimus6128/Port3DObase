#ifndef MAIN_3DO_H
#define MAIN_3DO_H

void effectSpritesGeckoInit(void);
void effectSpritesGeckoRun(void);

void effectLayersInit(void);
void effectLayersRun(void);

void effectParallaxInit(void);
void effectParallaxRun(void);

void effectJuliaInit(void);
void effectJuliaRun(void);

void effectWaterInit(void);
void effectWaterRun(void);

void effectSphereInit(void);
void effectSphereRun(void);

void effectFliAnimTestInit(void);
void effectFliAnimTestRun(void);

void effectAmvInit(void);
void effectAmvRun(void);

void effectRaytraceInit(void);
void effectRaytraceRun(void);

enum { EFFECT_SPRITES_GECKO, EFFECT_LAYERS, EFFECT_PARALLAX, EFFECT_JULIA, EFFECT_WATER, EFFECT_SPHERE, EFFECT_FLI_ANIM_TEST, EFFECT_AMV, EFFECT_RAYTRACE, EFFECTS_NUM };

int main3DO();

#endif
