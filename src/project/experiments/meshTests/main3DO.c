#include "core.h"
#include "tools.h"

#include "main3DO.h"

#ifdef PROJECT_3DO
	int main()
#else
	int main3DO()
#endif
{
	int extraOpts = (CORE_NO_VSYNC | CORE_VRAM_MAXBUFFERS);//CORE_SHOW_MEM;

	const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);
	//const int effectIndex = EFFECT_MESH_SOFT;

	if (effectIndex == EFFECT_MESH_SOFT || effectIndex == EFFECT_MESH_GOURAUD_CEL || effectIndex == EFFECT_MESH_GOURAUD_RGB_LIGHTS || effectIndex == EFFECT_MESH_WORLD) extraOpts |= CORE_INIT_3D_ENGINE_SOFT;
	if (effectIndex == EFFECT_MESH_GOURAUD_CEL || effectIndex == EFFECT_MESH_GOURAUD_RGB_LIGHTS) extraOpts |= CORE_INIT_SEMISOFT_GOURAUD;

	//if (effectIndex != EFFECT_MESH_PYRAMIDS) extraOpts |= (CORE_NO_VSYNC | CORE_VRAM_MAXBUFFERS);
	//if (effectIndex != EFFECT_MESH_FLI) extraOpts &= ~CORE_VRAM_MAXBUFFERS;


	coreInit(effectInitFunc[effectIndex], CORE_SHOW_FPS | CORE_INIT_3D_ENGINE | extraOpts);
	ScavengeMem();
	coreRun(effectRunFunc[effectIndex]);

	return 0;
}
