#ifndef EVENT_H
#define EVENT_H

#include "types.h"

enum ListenerCategory {
  LC_NoSeeUm         = 0,	/* Ignore all events */
  LC_FocusListener   = 1,	/* Receive events if I have focus	*/
  LC_Observer        = 2,	/* Receive events regardless of focus	*/
  LC_FocusUI         = 3	/* Receive UI events if I have focus	*/
				/* and other events regardless of focus */
};

#define ControlDown          0x80000000
#define ControlUp            0x40000000
#define ControlRight         0x20000000
#define ControlLeft          0x10000000
#define ControlA             0x08000000
#define ControlB             0x04000000
#define ControlC             0x02000000
#define ControlStart         0x01000000
#define ControlX             0x00800000
#define ControlRightShift    0x00400000
#define ControlLeftShift     0x00200000

#define MouseLeft            0x80000000
#define MouseMiddle          0x40000000
#define MouseRight           0x20000000
#define MouseShift           0x10000000


typedef struct ControlPadEventData {
	uint32         cped_ButtonBits;  /* left justified, zero fill */
} ControlPadEventData;

typedef struct MouseEventData {
	uint32         med_ButtonBits;   /* left justified, zero fill */
	int32          med_HorizPosition;
	int32          med_VertPosition;
} MouseEventData;

void InitEventUtility(int32 numControlPads, int32 numMice, int32 focusListener);

Err GetControlPad(int32 padNumber, int32 wait, ControlPadEventData* data);
Err GetMouse(int32 mouseNumber, int32 wait, MouseEventData* data);

#endif
