#include "core.h"
#include "tools.h"

#include "main3DO.h"

int main3DO()
{
	const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);

	coreInit(effectInitFunc[effectIndex], CORE_DEFAULT | CORE_SHOW_MEM);
	coreRun(effectRunFunc[effectIndex]);
}
