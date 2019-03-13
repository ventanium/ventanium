/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file url.h
 *
 * @brief URL parsing
 */

#ifndef VTM_NET_URL_H_
#define VTM_NET_URL_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char* VTM_URL_SCHEMES[];

enum vtm_url_scheme
{
	VTM_URL_SCHEME_HTTPS,
	VTM_URL_SCHEME_HTTP
};

struct vtm_url
{
	enum vtm_url_scheme	scheme;   /**< the scheme */
	char 				*host;    /**< host part */
	char 				*path;    /**< path part, maybe NULL */
	unsigned int 		port;     /**< port, maybe set to default scheme port */
};

/**
 * Parses an URL string.
 *
 * @param str the URL as NUL-terminated string
 * @param[out] url the url structure that is filled
 * @return VTM_OK if the URL was successfully parsed
 * @return VTM_E_INVALID_ARG if input is not a valid URL
 * @return VTM_E_NOT_SUPPRORTED if parts of the url are not supported yet,
 *         for example unknown schemes
 */
VTM_API int vtm_url_parse(const char *str, struct vtm_url *url);

/**
 * Releases all allocated resources of an url struct.
 *
 * @param url the url that should should be released
 */
VTM_API void vtm_url_release(struct vtm_url *url);

/**
 * URL encoding with percent sign.
 *
 * All characters except A-Z, a-Z, 0-9 and [-._~] are replaced with
 * the percent sign + their ASCII number.
 *
 * @param s the NUL-terminated input string
 * @param[out] buf holds the encoded NUL-terminated string
 * @param len the length of the buffer
 * @return VTM_OK if the encoding was successful
 * @return VTM_E_MAX_REACHED if the provided buffer is too small
 */
VTM_API int vtm_url_encode(const char *s, void *buf, size_t len);

/**
 * URL decoding with percent sign.
 *
 * All %xx patterns are replaced with the corresponding ASCII character.
 *
 * @param s the NUL-terminated input string
 * @param[out] buf holds the decoded NUL-terminated string
 * @param len the length of the buffer
 * @return VTM_OK if the decoding was successful
 * @return VTM_E_MAX_REACHED if the provided buffer is too small
 * @return VTM_E_INVALID_ARG if the input string was not properly encoded
 */
VTM_API int vtm_url_decode(const char *s, void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_URL_H_ */
