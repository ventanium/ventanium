/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file base64.h
 *
 * @brief BASE64 encoding and decoding
 */

#ifndef VTM_UTIL_BASE64_H_
#define VTM_UTIL_BASE64_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Calculates the necessary buffer size for encoding.
 * @param INPUT_LEN the input length
 */
#define VTM_BASE64_ENC_BUF_LEN(INPUT_LEN)   (4 * (((INPUT_LEN) + 2) / 3) + 1)

/**
 * Calculates the necessary buffer size for decoding.
 * @param INPUT_LEN length of BASE64 string
 */
#define VTM_BASE64_DEC_BUF_LEN(INPUT_LEN)   (((INPUT_LEN) / 4) * 3)

/**
 * Encodes a memory chunk into BASE64 format.
 *
 * The output is written including a final NUL-terminator.
 *
 * @param input pointer to input memory
 * @param len size of input
 * @param buf preallocated buffer where BASE64 string will be written to
 * @param buf_len size of buffer
 * @return VTM_OK if encoding was successful
 * @return VTM_E_INVALID_ARG if one of the supplied arguments is invalid,
 *         for example a too small buffer
 */
VTM_API int vtm_base64_encode(const void *input, size_t len, void *buf, size_t buf_len);

/**
 * Decodes a BASE64 string.
 *
 * @param input pointer to begin of BASE64 string
 * @param len length of input string
 * @param buf preallocated buffer where decoded data will be written to
 * @param[in, out] buf_len size of buffer, overwritten with used buffer size
 * @return VTM_OK if decoding was successful
 * @return VTM_E_INVALID_ARG if one of the supplied arguments is invalid,
 *         for example a too small buffer
 */
VTM_API int vtm_base64_decode(const void *input, size_t len, void *buf, size_t *buf_len);

#ifdef __cplusplus
}
#endif

#endif /* VTM_UTIL_BASE64_H_ */
