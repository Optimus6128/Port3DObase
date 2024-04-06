#ifndef IO_H
#define IO_H

#include "types.h"

/* common commands */
#define CMD_WRITE	0
#define CMD_READ	1
#define CMD_STATUS	2

typedef struct IOBuf
{
	void* iob_Buffer;	/* ptr to users buffer */
	int32	iob_Len;	/* len of this buffer, or transfer size */
} IOBuf;

/* User portion of IOReq data structure supplied by user*/
typedef struct IOInfo
{
	uint8	ioi_Command;	/* Command to be executed */
	uint8	ioi_Flags;	/* misc flags */
	uint8	ioi_Unit;	/* unit of this device */
	uint8	ioi_Flags2;	/* more flags, should be set to zero */
	uint32	ioi_CmdOptions;	/* device dependent options */
	uint32	ioi_User;	/* back ptr for user use */
	int32 	ioi_Offset;	/* offset into device for transfer to begin */
	IOBuf	ioi_Send;	/* copy out information */
	IOBuf	ioi_Recv;	/* copy in info, (address validated) */
} IOInfo;

void DoIO(Item ior, const IOInfo *ioiP);

#endif
