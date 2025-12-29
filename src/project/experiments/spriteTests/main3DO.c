#include "core.h"
#include "tools.h"

#include "main3DO.h"

// For some reason, those 3 defines below were in the header file like all the other experiment main3DO
// But of I move this one it has linker issues and that's normal as they use the same name (but why doesn't it happen with all the rest?)
// Anyway that's only because I was pressing F7 and building everything at once. If you do Ctrl+B after you set one of the projects as StartUp, it only builds that and it's ok.
// Which means I could move them back in their header, but I will be confused and press F7 and lose half of my hour, but also in the original 3DO they are in the header so I am not changing it for all the others.
// I want to keep the code as faithful as the original and maybe I should move those also to the header and just remember to press Ctrl+B and not just try to build all at once but only the selected project.

void(*effectInitFunc[EFFECTS_NUM])() = { effectSpritesGeckoInit, effectLayersInit, effectParallaxInit, effectJuliaInit, effectWaterInit, effectSphereInit, effectFliAnimTestInit, effectAmvInit, effectRaytraceInit };
void(*effectRunFunc[EFFECTS_NUM])() = { effectSpritesGeckoRun, effectLayersRun, effectParallaxRun, effectJuliaRun, effectWaterRun, effectSphereRun, effectFliAnimTestRun, effectAmvRun, effectRaytraceRun };
char* effectName[EFFECTS_NUM] = { "1920 gecko sprites", "background layers", "parallax tests", "julia fractal", "water ripples", "sphere mapping", "fli animation test", "AMV bits", "Raytracing" };

int main3DO()
{
	const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);
	int coreFlags = CORE_DEFAULT;

	if (effectIndex == EFFECT_FLI_ANIM_TEST) {
		coreFlags = CORE_SHOW_FPS | /*CORE_NO_VSYNC | */CORE_NO_CLEAR_FRAME;
	}

	if (effectIndex == EFFECT_RAYTRACE) {
		coreFlags = CORE_SHOW_FPS | CORE_SHOW_MEM | CORE_INIT_3D_ENGINE;
	}

	coreInit(effectInitFunc[effectIndex], coreFlags);
	coreRun(effectRunFunc[effectIndex]);

	return 0;
}
