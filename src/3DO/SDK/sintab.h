#ifndef SINTAB_H
#define SINTAB_H

#define SINE_LUT_BITS 12
#define SINE_LUT_SIZE (1 << SINE_LUT_BITS)
#define SINE_DEG_BITS 8

extern int sineLUT[SINE_LUT_SIZE];

#endif
