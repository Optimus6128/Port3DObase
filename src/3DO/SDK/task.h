#ifndef TASK_H
#define TASK_H

#include "types.h"

int32 WaitSignal(uint32 sigMask);
Err SendSignal(Item task, uint32 sigMask);
int32 AllocSignal(uint32 sigMask);
Err FreeSignal(uint32 sigMask);

#endif
