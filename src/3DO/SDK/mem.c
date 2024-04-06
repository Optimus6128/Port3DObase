#include "mem.h"

void AvailMem(MemInfo* minfo, uint32 flags)
{
	// Dummy

	// I forgot to set it in PC port, so debug worked and release failed because this was zero while in debug some big number.
	// I will just set a reference number a bit below 3MBs, then I will check on 3DO what the starting value of this is
	minfo->minfo_SysFree = 2500000;
}

void ScavengeMem()
{
	// Dummy
}
