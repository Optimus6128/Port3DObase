#ifndef __TYPES_3DO_H
#define __TYPES_3DO_H

typedef signed char	int8;
typedef signed short	int16;		/* Avoid this type - unreliable */
typedef signed long	int32;

typedef unsigned char	uchar;			/* should be unsigned char */
typedef uchar	ubyte;
typedef uchar	uint8;

typedef unsigned short	ushort;		/* Avoid this type - unreliable */
typedef ushort		uint16;		/* Avoid this type - unreliable */

typedef unsigned long	ulong;
typedef ulong		uint32;

typedef volatile long	vlong;
typedef	vlong		vint32;

typedef volatile unsigned long	vulong;
typedef	vulong			vuint32;

typedef	uint32	Boolean;
typedef Boolean	bool;

#define TRUE	((Boolean)1)
#define FALSE	((Boolean)0)

#define false	FALSE
#define true	TRUE

typedef int32	Item;
typedef	int32	Err;

#endif
