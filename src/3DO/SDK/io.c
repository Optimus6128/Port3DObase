#include "io.h"

#include "graphics.h"

#include <string.h>


void DoIO(Item ior, const IOInfo *ioiP)
{
	switch(ioiP->ioi_Command) {
		case FLASHWRITE_CMD:
		{
			uint32* dst = (uint32*)ioiP->ioi_Recv.iob_Buffer;
			int32 length = ioiP->ioi_Recv.iob_Len / 4;
			uint32 value = (uint32)ioiP->ioi_Offset;
			do {
				*dst++ = value;
			} while (--length != 0);
		}
		break;

		case SPORTCMD_COPY:
		{
			Item vsyncItem = GetVBLIOReq();
			WaitVBL(vsyncItem, 1);
			memcpy(ioiP->ioi_Recv.iob_Buffer, ioiP->ioi_Send.iob_Buffer, ioiP->ioi_Send.iob_Len);
		}
		break;
	}
}
