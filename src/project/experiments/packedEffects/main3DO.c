#include "core.h"
#include "tools.h"

#include "main3DO.h"

#ifdef PROJECT_3DO
	int main()
#else
	int main3DO()
#endif
{
	const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);

	coreInit(effectInitFunc[effectIndex], CORE_DEFAULT | CORE_SHOW_MEM);
	coreRun(effectRunFunc[effectIndex]);

	return 0;
}
