#include "event.h"

#include "../main.h"

#include <string.h>


static ControlPadEventData globalControlPadEventData;
static MouseEventData globalMouseEventData;

void InitEventUtility(int32 numControlPads, int32 numMice, int32 focusListener)
{
	globalControlPadEventData.cped_ButtonBits = 0;
}

Err GetControlPad(int32 padNumber, int32 wait, ControlPadEventData* data)
{
	update3DOinputSDL(&globalControlPadEventData, &globalMouseEventData);
	memcpy(data, &globalControlPadEventData, sizeof(ControlPadEventData));
	return 0;
}

Err GetMouse(int32 mouseNumber, int32 wait, MouseEventData* data)
{
	update3DOinputSDL(&globalControlPadEventData, &globalMouseEventData);
	memcpy(data, &globalMouseEventData, sizeof(MouseEventData));
	return 0;
}
