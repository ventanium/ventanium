/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "math.h"

#include <math.h>
#include <vtm/core/error.h>

int vtm_math_size_add(size_t a, size_t b, size_t *r)
{
	size_t res;

	res = a + b;
	if (res < a)
		return VTM_E_OVERFLOW;

	*r = res;
	return VTM_OK;
}

int vtm_math_size_mul(size_t a, size_t b, size_t *r)
{
	size_t res;

	res = a * b;
	if (res < a)
		return VTM_E_OVERFLOW;

	*r = res;
	return VTM_OK;
}

bool vtm_math_float_cmp(float a, float b, float epsilon)
{
	return fabsf(a-b) < epsilon;
}

bool vtm_math_double_cmp(double a, double b, double epsilon)
{
	return fabs(a-b) < epsilon;
}
