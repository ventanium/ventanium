/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file format.h
 *
 * @brief Printing different types as string
 */

#ifndef VTM_CORE_FORMAT_H_
#define VTM_CORE_FORMAT_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum needed buffer size for storing a 32 bit integer value as string */
#define VTM_FMT_CHARS_INT32         11

/** Maximum needed buffer size for storing a 64 bit integer value as string */
#define VTM_FMT_CHARS_INT64         20

/** Maximum needed buffer size for storing a 64 bit size_t value as hex-string */
#define VTM_FMT_CHARS_HEX_SIZE      16

/** Uppercase hex chars */
VTM_API extern const char VTM_HEX_CHARS[];

/** Lowercase hex chars */
VTM_API extern const char VTM_HEX_LCHARS[];

/**
 * Print string representation of signed integer.
 *
 * @param dst the destination where the string is written to, can be NULL
 * @param val the value that should be printed
 * @return the number of characters used to represent the number
 */
VTM_API unsigned int vtm_fmt_int(char *dst, int val);

/**
 * Print string representation of unsigned integer.
 *
 * @param dst the destination where the string is written to, can be NULL
 * @param val the value that should be printed
 * @return the number of characters used to represent the number
 */
VTM_API unsigned int vtm_fmt_uint(char *dst, unsigned int val);

/**
 * Print string representation of signed 64 bit integer.
 *
 * @param dst the destination where the string is written to, can be NULL
 * @param val the value that should be printed
 * @return the number of characters used to represent the number
 */
VTM_API unsigned int vtm_fmt_int64(char *dst, int64_t val);

/**
 * Print string representation of unsigned 64 bit integer.
 *
 * @param dst the destination where the string is written to, can be NULL
 * @param val the value that should be printed
 * @return the number of characters used to represent the number
 */
VTM_API unsigned int vtm_fmt_uint64(char *dst, uint64_t val);

/**
 * Print hex-string representation of size_t value.
 *
 * @param dst the destination where the string is written to, can be NULL
 * @param val the value that should be printed
 * @return the number of characters used by the hex-string
 */
VTM_API unsigned int vtm_fmt_hex_size(char *dst, size_t val);

/**
 * Print hex-string representation of a memory chunk.
 *
 * @param dst the destination where the string is written to, can be NULL
 * @param src pointer to memory chunk
 * @param len length of memory chunk in bytes
 * @param ucase true if uppercase hex letters should be used
 * @return the number of characters used by the hex-string
 */
VTM_API unsigned int vtm_fmt_hex_mem(char *dst, const unsigned char *src, size_t len, bool ucase);

#ifdef __cplusplus
}
#endif

#endif /* VTM_CORE_FORMAT_H_ */
