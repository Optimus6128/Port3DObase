#include "core.h"

#include "input.h"


// order must correspond to enum order
static int joyButtonHwIDs[JOY_BUTTONS_NUM] = { ControlUp, ControlDown, ControlLeft, ControlRight, ControlA, ControlB, ControlC, ControlLeftShift, ControlRightShift, ControlX, ControlStart };
static int mouseButtonHwIDs[MOUSE_BUTTONS_NUM] = { MouseLeft, MouseMiddle, MouseRight };

static bool joyButtonPressed[JOY_BUTTONS_NUM];
static bool joyButtonPressedOnce[JOY_BUTTONS_NUM];

static bool mouseButtonPressed[MOUSE_BUTTONS_NUM];
static bool mouseButtonPressedOnce[MOUSE_BUTTONS_NUM];

static MousePosition previousMousePosition;
static MousePosition mousePosition;
static int mouseStatus = 0;
static bool anyJoyButtonPressed;

void initInput()
{
	int i;

	for (i=0; i<JOY_BUTTONS_NUM; ++i) {
		joyButtonPressed[i] = false;
		joyButtonPressedOnce[i] = false;
	}

	for (i=0; i<MOUSE_BUTTONS_NUM; ++i) {
		mouseButtonPressed[i] = false;
		mouseButtonPressedOnce[i] = false;
	}

	mousePosition.x = 0;
	mousePosition.y = 0;
}

static void updateJoypad()
{
	int i, joybits;

	ControlPadEventData cpaddata;
	cpaddata.cped_ButtonBits=0;
	GetControlPad(1,0,&cpaddata);

	joybits = cpaddata.cped_ButtonBits;

	anyJoyButtonPressed = false;
	for (i=0; i<JOY_BUTTONS_NUM; ++i) {
		if (joybits & joyButtonHwIDs[i]) {
			joyButtonPressedOnce[i] = !joyButtonPressed[i];
			joyButtonPressed[i] = true;
			anyJoyButtonPressed = true;
		} else {
			joyButtonPressed[i] = false;
			joyButtonPressedOnce[i] = false;
		}
	}
}

static void updateMouse()
{
	int i, mousebits;

	MouseEventData mouseState;

    mouseStatus = GetMouse(1,0,&mouseState);
	if (mouseStatus < 0) return;

	previousMousePosition.x = mousePosition.x;
	previousMousePosition.y = mousePosition.y;

	mousePosition.x = mouseState.med_HorizPosition;
	mousePosition.y = mouseState.med_VertPosition;

	mousebits = mouseState.med_ButtonBits;

	for (i=0; i<MOUSE_BUTTONS_NUM; ++i) {
		if (mousebits & mouseButtonHwIDs[i]) {
			mouseButtonPressedOnce[i] = !mouseButtonPressed[i];
			mouseButtonPressed[i] = true;
		} else {
			mouseButtonPressed[i] = false;
			mouseButtonPressedOnce[i] = false;
		}
	}
}

MousePosition getMousePositionDiff()
{
	static MousePosition mousePositionDiff;

	mousePositionDiff.x = mousePosition.x - previousMousePosition.x;
	mousePositionDiff.y = mousePosition.y - previousMousePosition.y;

	return mousePositionDiff;
}

MousePosition getMousePosition()
{
	return mousePosition;
}

void updateInput()
{
	updateJoypad();
	updateMouse();
}

bool isAnyJoyButtonPressed()
{
	return anyJoyButtonPressed;
}

bool isJoyButtonPressed(int joyButtonId)
{
	if (joyButtonId < 0 || joyButtonId >= JOY_BUTTONS_NUM) return false;
	return joyButtonPressed[joyButtonId];
}

bool isJoyButtonPressedOnce(int joyButtonId)
{
	if (joyButtonId < 0 || joyButtonId >= JOY_BUTTONS_NUM) return false;
	return joyButtonPressedOnce[joyButtonId];
}

bool isMouseButtonPressed(int mouseButtonId)
{
	if (mouseButtonId < 0 || mouseButtonId >= MOUSE_BUTTONS_NUM) return false;
	return mouseButtonPressed[mouseButtonId];
}

bool isMouseButtonPressedOnce(int mouseButtonId)
{
	if (mouseButtonId < 0 || mouseButtonId >= MOUSE_BUTTONS_NUM) return false;
	return mouseButtonPressedOnce[mouseButtonId];
}
