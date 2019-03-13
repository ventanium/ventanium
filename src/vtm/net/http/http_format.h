/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file http_format.h
 *
 * @brief Header value formatting
 */

#ifndef VTM_NET_HTTP_HTTP_FORMAT_H_
#define VTM_NET_HTTP_HTTP_FORMAT_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/util/time.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Minimum buffer length in bytes for holding a formatted HTTP date */
#define VTM_HTTP_DATE_LEN       30

/**
 * Converts given date to HTTP date format.
 *
 * @param dst the buffer where the converted date string is stored
 * @param max_lan the length of the buffer in bytes
 * @param date the date that should be converted
 * @return VTM_OK if the conversion was successful
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_http_fmt_date(char *dst, size_t max_len, struct vtm_date *date);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_FORMAT_H_ */
