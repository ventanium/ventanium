/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "serialization.h"

#include <math.h>
#include <float.h>
#include <vtm/core/error.h>

#define VTM_MANTISSA_23      8388606             /**< 2^23 - 2 */
#define VTM_MANTISSA_52      4503599627370494ULL /**< 2^52 - 2 */

#define VTM_FLT_EXP_OFFSET   (-FLT_MIN_EXP)
#define VTM_DBL_EXP_OFFSET   (-DBL_MIN_EXP)

int vtm_pack_float(float in, uint32_t *out)
{
	int exp;
	float frac;
	uint32_t mantissa;

	if (isnan(in))
		return VTM_E_INVALID_ARG;

	frac = frexpf(in, &exp);
	mantissa = (frac == 0.0f) ? 0 : (uint32_t) (1 + (fabsf(frac) - 0.5f) * 2 * VTM_MANTISSA_23);

	*out = ((in < 0) ? 1 << 31 : 0) + (mantissa << 8) + (uint8_t) (exp + VTM_FLT_EXP_OFFSET);

	return VTM_OK;
}

int vtm_pack_double(double in, uint64_t *out)
{
	int exp;
	double frac;
	uint64_t mantissa;

	if (isnan(in))
		return VTM_E_INVALID_ARG;

	frac = frexp(in, &exp);
	mantissa = (frac == 0.0) ? 0 : (uint64_t) (1 + (fabs(frac) - 0.5) * 2 * VTM_MANTISSA_52);

	*out = ((in < 0) ? 1ULL << 63 : 0) + (mantissa << 11) + (uint16_t) (exp + VTM_DBL_EXP_OFFSET);

	return VTM_OK;
}

int vtm_unpack_float(uint32_t in, float *out)
{
	int exp;
	float sig, mantissa;
	uint32_t man;

	sig = in >> 31 ? -1.0f : 1.0f;
	exp = (int) (in & 0xff) - VTM_FLT_EXP_OFFSET;
	man = (in & 0x7fffffff) >> 8;

	if (man < 1) {
		*out = sig * 0.0f;
		return VTM_OK;
	}

	mantissa = (float) (man - 1) / VTM_MANTISSA_23 / 2.0f;
	*out = sig * ldexpf(mantissa + 0.5f, exp);

	return VTM_OK;
}

int vtm_unpack_double(uint64_t in, double *out)
{
	int exp;
	double sig, mantissa;
	uint64_t man;

	sig =  in >> 63 ? -1.0 : 1.0;
	exp = (int) (in & 0x7ff) - VTM_DBL_EXP_OFFSET;
	man = (in & 0x7fffffffffffffff) >> 11;

	if (man < 1) {
		*out = sig * 0.0;
		return VTM_OK;
	}

	mantissa = (double) (man - 1) / VTM_MANTISSA_52 / 2.0;
	*out = sig * ldexp(mantissa + 0.5, exp);

	return VTM_OK;
}
