#include "core.h"
#include "tools.h"

#include "main3DO.h"

int main3DO()
{
	const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);
	int coreFlags = CORE_DEFAULT;

	if (effectIndex == EFFECT_FLI_ANIM_TEST) {
		coreFlags = CORE_SHOW_FPS | CORE_NO_VSYNC | CORE_NO_CLEAR_FRAME;
	}

	coreInit(effectInitFunc[effectIndex], coreFlags);
	coreRun(effectRunFunc[effectIndex]);
}
