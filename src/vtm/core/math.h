/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file math.h
 *
 * @brief Math functions like overflow aware addition
 */

#ifndef VTM_CORE_MATH_H_
#define VTM_CORE_MATH_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Evaluates to the smaller argument */
#define VTM_MIN(a, b) ((a) < (b) ? (a) : (b))

/** Evaluates to the greater argument */
#define VTM_MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * Overflow aware addition of two size_t values.
 *
 * @param a first value
 * @param b second value
 * @param[out] r the result of the addion
 * @return VTM_OK if the addition has not resulted in an overflow
 * @return VTM_E_OVERFLOW if the addition would cause an overflow
 */
VTM_API int vtm_math_size_add(size_t a, size_t b, size_t *r);

/**
 * Overflow aware multiplication of two size_t values.
 *
 * @param a the multiplicand
 * @param b the multiplier
 * @param[out] r the result
 * @return VTM_OK if the multiplication has not resulted in an overflow
 * @return VTM_E_OVERFLOW if the multiplication would cause an overflow
 */
VTM_API int vtm_math_size_mul(size_t a, size_t b, size_t *r);

/**
 * Compares two float values for (relative) equality.
 *
 * @param a the first input float
 * @param b the second input float
 * @param epsilon the precision threshold
 */
VTM_API bool vtm_math_float_cmp(float a, float b, float epsilon);

/**
 * Compares two double values for (relative) equality.
 *
 * @param a the first input double
 * @param b the second input double
 * @param epsilon the precision threshold
 */
VTM_API bool vtm_math_double_cmp(double a, double b, double epsilon);

#ifdef __cplusplus
}
#endif

#endif /* VTM_CORE_MATH_H_ */
