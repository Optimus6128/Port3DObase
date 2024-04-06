#include "timerutils.h"

#include <SDL2/SDL.h>

Item GetTimerIOReq()
{
	// Dummy
	return 0;
}

int32 GetMSecTime(Item ioreq)
{
	return SDL_GetTicks();
}
