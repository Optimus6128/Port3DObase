#ifndef MEM_H
#define MEM_H

#include "types.h"


/* define location, size flags */
#define MEMTYPE_ANY		(uint32)0

/* low 8 bits are optional fill value */
#define MEMTYPE_MASK		(uint32)0xffffff00
#define MEMTYPE_FILLMASK	(uint32)0x000000ff

#define MEMTYPE_FILL		(uint32)0x00000100 /* fill memory with value */
#define MEMTYPE_MYPOOL		(uint32)0x00000200 /* do not get more memory from system */
#define MEMTYPE_FROMTOP		(uint32)0x00004000 /* allocate from top      */
#define MEMTYPE_TASKMEM		(uint32)0x00008000 /* internal use only      */
#define MEMTYPE_TRACKSIZE	(uint32)0x00400000 /* track allocation size  */

/* memory type bits */
#define MEMTYPE_DMA		(uint32)0x00020000 /* accessable by hardware    */
#define MEMTYPE_CEL		(uint32)0x00040000 /* accessable by cel engine  */
#define MEMTYPE_DRAM		(uint32)0x00080000 /* block must not be in VRAM */
#define MEMTYPE_AUDIO		(uint32)0x00100000 /* accessible by audio       */
#define MEMTYPE_DSP		(uint32)0x00200000 /* accessible by DSP         */
#define MEMTYPE_VRAM		(uint32)0x00010000 /* block must be in VRAM     */
#define MEMTYPE_VRAM_BANK1	(MEMTYPE_VRAM | MEMTYPE_BANKSELECT | MEMTYPE_BANK1)
#define MEMTYPE_VRAM_BANK2	(MEMTYPE_VRAM | MEMTYPE_BANKSELECT | MEMTYPE_BANK2)

/* alignment bits */
#define MEMTYPE_INPAGE		(uint32)0x01000000 /* no page crossings */
#define MEMTYPE_STARTPAGE	(uint32)0x02000000 /* block must start on VRAM boundary */
#define MEMTYPE_SYSTEMPAGESIZE	(uint32)0x04000000 /* block must start on system page boundary */

/* If MEMTYPE_VRAM is set, PAGESIZE = (VRAM PAGESIZE),
 *              otherwise, PAGESIZE = (physical page size of mem protect system)
 */

#define MEMF_ALIGNMENTS	(MEMTYPE_INPAGE|MEMTYPE_STARTPAGE)

 /* VRAM bank select bits */
#define MEMTYPE_BANKSELECT	(uint32)0x40000000   /* bank required */
#define MEMTYPE_BANKSELECTMSK	(uint32)0x30000000   /* 2 max banks   */
#define MEMTYPE_BANK1		(uint32)0x10000000   /* first bank    */
#define MEMTYPE_BANK2		(uint32)0x20000000   /* second bank   */ 


/* ControlMem() commands */
#define MEMC_NOWRITE	1	/* make memory unwritable for a task   */
#define MEMC_OKWRITE	2	/* make memory writable for a task     */
#define MEMC_GIVE	3	/* give memory away, or back to system */
#define MEMC_SC_GIVE    4       /* special give for ScavengeMem()      */

#define AllocMem(a,b) malloc(a)
#define FreeMem(a,b) free(a)

/* Structure passed to AvailMem() */
typedef struct MemInfo
{
	uint32  minfo_SysFree;     /* Bytes free in system memory (free pages) */
	uint32  minfo_SysLargest;  /* Largest span of free system memory pages */
	uint32  minfo_TaskFree;    /* Bytes of "odds & ends" in task memory    */
	uint32  minfo_TaskLargest; /* Largest "odd or end" in task memory      */
} MemInfo;

void AvailMem(MemInfo* minfo, uint32 flags);
void ScavengeMem();

#endif
