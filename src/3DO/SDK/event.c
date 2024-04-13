#include "event.h"

#include "../main.h"

#include <string.h>


ControlPadEventData globalControlPadEventData;
MouseEventData globalMouseEventData;

void InitEventUtility(int32 numControlPads, int32 numMice, int32 focusListener)
{
	globalControlPadEventData.cped_ButtonBits = 0;
}

Err GetControlPad(int32 padNumber, int32 wait, ControlPadEventData* data)
{
	update3DOinputSDL(&globalControlPadEventData);
	memcpy(data, &globalControlPadEventData, sizeof(ControlPadEventData));
	return 0;
}

Err GetMouse(int32 mouseNumber, int32 wait, MouseEventData* data)
{
	// TODO
	return 0;
}
