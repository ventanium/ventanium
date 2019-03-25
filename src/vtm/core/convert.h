/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

/**
 * @file convert.h
 *
 * @brief Type conversion
 */

#ifndef VTM_CORE_CONVERT_H_
#define VTM_CORE_CONVERT_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the string representation of a signed 64 bit integer.
 *
 * @param num the integer that should be represented as string
 * @return a string containing the number
 * @return NULL if memory allocation failed
 */
VTM_API char* vtm_conv_int64_str(int64_t num);

/**
 * Returns the string representation of an unsigned 64 bit integer.
 *
 * @param num the integer that should be represented as string
 * @return a string containing the number
 * @return NULL if memory allocation failed
 */
VTM_API char* vtm_conv_uint64_str(uint64_t num);

/**
 * Returns the string representation of double value.
 *
 * @param r the double that should be represented as string
 * @return a string containing the double in text form
 * @return NULL if memory allocation failed
 */
VTM_API char* vtm_conv_double_str(double r);

/**
 * Returns a hex-string representation of the blob contents.
 *
 * @param blob the blob that should be represented as string
 * @return a hex-string describing the blob memory
 * @return NULL if memory allocation failed
 */
VTM_API char* vtm_conv_blob_str(void *blob);

/**
 * Parses signed 64 bit integer value from string.
 *
 * @param in the NUL-terminated input string
 * @return the converted number
 */
VTM_API int64_t vtm_conv_str_int64(const char *in);

/**
 * Parses unsigned 64 bit integer value from string.
 *
 * @param in the NUL-terminated input string
 * @return the converted number
 */
VTM_API uint64_t vtm_conv_str_uint64(const char *in);

/**
 * Parses a double value from string.
 *
 * @param in the NUL-terminated input string
 * @return the converted double
 */
VTM_API double vtm_conv_str_double(const char *in);

#ifdef __cplusplus
}
#endif

#endif /* VTM_CORE_CONVERT_H_ */
