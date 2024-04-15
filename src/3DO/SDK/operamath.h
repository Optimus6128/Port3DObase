#ifndef OPERAMATH_H
#define OPERAMATH_H

#include "types.h"
#include "sintab.h"

#define OPERA_MATH_FP 16

/*
 * frac16 is a 16.16 fraction
 * frac32 is a 32.32 fraction
 * frac30 is a 2.30 fraction
 * frac60 is a 4.60 fraction
 * frac14 is a 2.14 fraction (using the upper 16 bits of a 32 bit quantity)
 */

typedef int32 frac16, frac30, frac14;
typedef uint32 ufrac16, ufrac30, ufrac14;
typedef struct int64 { uint32 hi; uint32 lo; } int64, uint64, frac32, ufrac32, frac60, ufrac60;

typedef frac16 vec3f16[3], vec4f16[4], mat33f16[3][3], mat44f16[4][4], mat34f16[4][3];
typedef frac30 vec3f30[3], vec4f30[4], mat33f30[3][3], mat44f30[4][4], mat34f30[4][3];


#define SinF16(a) sineLUT[((a) & ((1 << 24) - 1)) >> (16 - (SINE_LUT_BITS - SINE_DEG_BITS))]
#define CosF16(a) SinF16((a) + (64 << 16))

#define Atan2F16(a,b) (int)(atan2((double)(a) / 65536.0, (double)(b) / 65536.0) * 65536.0)

void MulManyVec3Mat33_F16(vec3f16* dest, vec3f16* src, mat33f16 mat, int32 count);
void MulVec3Mat33_F16(vec3f16 dest, vec3f16 vec, mat33f16 mat);
void MulMat33Mat33_F16(mat33f16 dest, mat33f16 src1, mat33f16 src2);

void MulManyVec4Mat44_F16(vec4f16* dest, vec4f16* src, mat44f16 mat, int32 count);

void OpenMathFolio();

#endif
