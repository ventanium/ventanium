/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file http_request.h
 *
 * @brief HTTP request
 */

#ifndef VTM_NET_HTTP_HTTP_REQUEST_H_
#define VTM_NET_HTTP_HTTP_REQUEST_H_

#include <vtm/core/api.h>
#include <vtm/core/dataset.h>
#include <vtm/core/types.h>
#include <vtm/net/http/http.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vtm_http_req
{
	enum vtm_http_method    method;   /**< method of the request */
	enum vtm_http_version   version;  /**< HTTP version */
	const char             *path;     /**< request path without parameters */
	vtm_dataset            *headers;  /**< headers, keys are case insensitive */
	vtm_dataset            *params;   /**< parameters that were encoded in url */
	void                   *con;      /**< internal */
};

/**
 * Convenience method for retrieving the host header.
 *
 * @param req the request
 * @return the host header value
 * @return NULL if host header was not present
 */
VTM_API const char* vtm_http_req_get_host(struct vtm_http_req *req);

/**
 * Convenience method for retrieving a header value as string.
 *
 * @param req the request
 * @param name the name of the header
 * @return the header value
 * @return NULL if the header field was not present
 */
VTM_API const char* vtm_http_req_get_header_str(struct vtm_http_req *req, const char *name);

/**
 * Convenience method for retrieving a URL parameter value as string.
 *
 * @param req the request
 * @param name the name of the parameter
 * @return the parameter value
 * @return NULL if the parameter was not set
 */
VTM_API const char* vtm_http_req_get_query_str(struct vtm_http_req *req, const char *name);

/**
 * Retrieves the source ip address and used port of a request.
 *
 * @param req the request whose source address should be retrieved
 * @param[out] buf the buffer where the ip address string representation is
 *             stored
 * @param len the length of the buffer in bytes
 * @param[out] port the used port number
 * @return VTM_OK if the address was successfully retrieved
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_http_req_get_remote_info(struct vtm_http_req *req, char *buf, size_t len, unsigned int *port);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_REQUEST_H_ */
