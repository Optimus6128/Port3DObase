#include "operamath.h"

void MulManyVec3Mat33_F16(vec3f16* dest, vec3f16* src, mat33f16 mat, int32 count)
{
	int i;
	frac16 *m = (frac16*)mat;
	frac16 *s = (frac16*)src;
	frac16 *d = (frac16*)dest;

	for (i = 0; i < count; ++i)
	{
		const int x = *s++;
		const int y = *s++;
		const int z = *s++;

		*d++ = (frac16)( ((long long int)x * (long long int)m[0] + (long long int)y * (long long int)m[3] + (long long int)z * (long long int)m[6]) >> OPERA_MATH_FP);
		*d++ = (frac16)( ((long long int)x * (long long int)m[1] + (long long int)y * (long long int)m[4] + (long long int)z * (long long int)m[7]) >> OPERA_MATH_FP);
		*d++ = (frac16)( ((long long int)x * (long long int)m[2] + (long long int)y * (long long int)m[5] + (long long int)z * (long long int)m[8]) >> OPERA_MATH_FP);
	}
}

void MulVec3Mat33_F16(vec3f16 dest, vec3f16 vec, mat33f16 mat)
{
	MulManyVec3Mat33_F16((vec3f16*)dest, (vec3f16*)vec, mat, 1);
}

void MulMat33Mat33_F16(mat33f16 dest, mat33f16 src1, mat33f16 src2)
{
	frac16* m1 = (frac16*)src1;
	frac16* m2 = (frac16*)src2;
	frac16* mDst = (frac16*)dest;

	for (int k = 0; k < 3; ++k) {
		for (int j = 0; j < 3; ++j) {
			frac16 sum = 0;
			for (int i = 0; i < 3; ++i) {
				sum += (frac16)( ((long long int)m1[3 * j + i] * (long long int)m2[3 * i + k]) >> OPERA_MATH_FP);
			}
			mDst[3 * j + k] = sum;
		}
	}
}

void OpenMathFolio()
{
	// Dummy
}
