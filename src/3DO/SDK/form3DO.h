#ifndef FORM3DO_H
#define FORM3DO_H

#include "types.h"
#include "graphics.h"

#define kCLUTWords 32

typedef struct VDL_REC
{
	uint32	controlword;					/*	VDL display control word (+ number of int32words in this entry - 4) */
											/*	(+ number of lines that this vdl is in effect -1) */
	uint32	curLineBuffer;					/*	1st byte of frame buffer */
	uint32	prevLineBuffer; 				/*	1st byte of frame buffer */
	uint32	nextVDLEntry;					/*	GrafBase->gf_VDLPostDisplay for last VDL Entry */
	uint32	displayControl; 				/*	Setup control info: DEFAULT_DISPCTRL */
	uint32	CLUTEntry[kCLUTWords];			/*	32 Clut entries for each R, G, and B */
	uint32	backgroundEntry;				/*	RGB 000 will use this entry */
	uint32	filler1;						/*	need 40 entries for now, hardware bug */
	uint32	filler2;
} VDL_REC;

typedef struct VdlChunk 		/* used for a standard 33 entry vdl list */
	{
	int32	chunk_ID;			/* 'VDL ' Magic number to identify VDL chunk */
	int32	chunk_size; 		/*	size in bytes of chunk including chunk_size */
	int32	vdlcount;			/*	count of number of vdls following */
	VDL_REC vdl[1]; 			/*	VDL control words and entries  */
} VdlChunk;


// Note. Previously I would flip endianess on the chunk_ID inside getChunk to compare with the values below.
// That triggered a false positive virus block on microsoft defender.
// So I do it the other way around for the comparison to possibly avoid that crap.
#ifdef BIG_ENDIAN
	#define CHAR4LITERAL(a,b,c,d)	((unsigned long) (a<<24)|(b<<16)|(c<<8)|d)
#else
	#define CHAR4LITERAL(a,b,c,d)	((unsigned long) (d<<24)|(c<<16)|(b<<8)|a)
#endif

#define CHUNK_3DO		CHAR4LITERAL('3','D','O',' ')   /* '3DO ' wrapper chunk */
#define CHUNK_IMAG		CHAR4LITERAL('I','M','A','G')   /* 'IMAG' the image control chunk */
#define CHUNK_PDAT		CHAR4LITERAL('P','D','A','T')   /* 'PDAT' pixel data */
#define CHUNK_CCB		CHAR4LITERAL('C','C','B',' ')   /* 'CCB ' cel control */
#define CHUNK_ANIM		CHAR4LITERAL('A','N','I','M')   /* 'ANIM' ANIM chunk */
#define CHUNK_PLUT		CHAR4LITERAL('P','L','U','T')   /* 'PLUT' pixel lookup table */
#define CHUNK_VDL		CHAR4LITERAL('V','D','L',' ')   /* 'VDL ' VDL chunk */
#define CHUNK_CPYR		CHAR4LITERAL('C','P','Y','R')   /* 'CPYR' copyright text*/
#define CHUNK_DESC		CHAR4LITERAL('D','E','S','C')   /* 'DESC' description text */
#define CHUNK_KWRD		CHAR4LITERAL('K','W','R','D')   /* 'KWRD' keyword text */
#define CHUNK_CRDT		CHAR4LITERAL('C','R','D','T')   /* 'CRDT' credits text */
#define CHUNK_XTRA		CHAR4LITERAL('X','T','R','A')   /* 'XTRA' 3DO Animator creates these */

/* Cel Control Block Chunk	 */
typedef struct CCC
{
	int32	chunk_ID;			/* 'CCB ' Magic number to identify pixel data */
	int32	chunk_size; 		/* size in bytes of chunk including chunk_size */
	uint32	ccbversion; 		/* version number of the scob data structure.  0 for now */
	uint32	ccb_Flags;			/* 32 bits of CCB flags */
	struct	CCB* ccb_NextPtr;
	CelData* ccb_CelData;
	void* ccb_PIPPtr; 	/* This will change to ccb_PLUTPtr in the next release */

	Coord	ccb_X;
	Coord	ccb_Y;
	int32	ccb_hdx;
	int32	ccb_hdy;
	int32	ccb_vdx;
	int32	ccb_vdy;
	int32	ccb_ddx;
	int32	ccb_ddy;
	uint32	ccb_PPMPC;
	uint32	ccb_PRE0;			/* Sprite Preamble Word 0 */
	uint32	ccb_PRE1;			/* Sprite Preamble Word 1 */
	int32	ccb_Width;
	int32	ccb_Height;
} CCC;


typedef uint16	RGB555;

typedef struct PixelChunk
{
	int32	chunk_ID;				/* 'PDAT' Magic number to identify pixel data */
	int32	chunk_size; 			/*	size in bytes of chunk including chunk_size */
	uint8	pixels[1];				/*	pixel data (format depends upon description in the imagehdr */
} PixelChunk;

typedef struct PLUTChunk
{
	int32	chunk_ID;			/* 'PLUT' Magic number to identify pixel data */
	int32	chunk_size; 		/*	size in bytes of chunk including chunk_size */
	int32	numentries; 		/*	number of entries in PLUT Table */
	RGB555	PLUT[1];			/*	PLUT entries  */
} PLUTChunk;

char* GetChunk(uint32* chunk_ID, char** buffer, int32* bufLen);

#endif
