/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file http_client.h
 *
 * @brief HTTP client
 */

#ifndef VTM_NET_HTTP_HTTP_CLIENT_H_
#define VTM_NET_HTTP_HTTP_CLIENT_H_

#include <vtm/core/api.h>
#include <vtm/core/dataset.h>
#include <vtm/core/types.h>
#include <vtm/net/network.h>
#include <vtm/net/socket_addr.h>
#include <vtm/net/http/http.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VTM_HTTP_CL_OPT_NO_CERT_CHECK    1  /**< expects bool */
#define VTM_HTTP_CL_OPT_TIMEOUT          2  /**< expects unsigned long, value is millisceonds */

/** HTTP client request */
struct vtm_http_client_req
{
	enum vtm_http_method   method;       /**< HTTP method */
	enum vtm_http_version  version;      /**< HTTP protocol version */
	enum vtm_socket_family fam;          /**< Socket family, IPv4 or IPv6 */
	const char             *url;         /**< URL of the request */
	vtm_dataset            *headers;     /**< Additional headers, can be NULL */
	const void             *body;        /**< Pointer to body data, can be NULL */
	size_t                 body_len;     /**< Length of body data, can be zero */
};

/** HTTP client response */
struct vtm_http_client_res
{
	enum vtm_http_version  version;      /**< HTTP protocol version */
	int                    status_code;  /**< Status code */
	const char             *status_msg;  /**< Status message */
	vtm_dataset            *headers;     /**< Response headers */
	void                   *body;        /**< Pointer to body data, maybe NULL */
	uint64_t               body_len;     /**< Length of body data, maybe zero */
};

typedef struct vtm_http_client vtm_http_client;

/**
 * Creates a new client.
 *
 * @return the created client which can be used in the other functions
 * @return NULL if an error occured
 */
VTM_API vtm_http_client* vtm_http_client_new(void);

/**
 * Releases the client and all allocated resources.
 *
 * After this call the client pointer is no longer valid.
 *
 * @param cl the client that should be released
 */
VTM_API void vtm_http_client_free(vtm_http_client *cl);

/**
 * Sets one of the possible options.
 *
 * The possible options are macros starting with VTM_HTTP_CL_OPT_.
 *
 * @param cl the client where the option should be set
 * @param opt the option that should be set
 * @param val pointer to new value of the option
 * @param len size of the value
 * @return VTM_OK if the option was successfully set
 * @return VTM_E_NOT_SUPPORTED if the given option or the value format is
 *         not supported
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_http_client_set_opt(vtm_http_client *cl, int opt, const void *val, size_t len);

/**
 * Sends specified HTTP request and retrieves response.
 *
 * This call blocks until the response was received or the timeout elapsed.
 *
 * @param cl the client that should make the request
 * @param req the request parameters
 * @param[out] res the response is stored here
 * @return VTM_OK if the call succeeded and a response was received
 * @return VTM_E_IO_TIMEOUT if the timeout elapsed before the response
 *         was received
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_http_client_request(vtm_http_client *cl, struct vtm_http_client_req *req, struct vtm_http_client_res *res);

/**
 * Releases all allocated resources of the given response.
 *
 * @param res the response that should be released
 */
VTM_API void vtm_http_client_res_release(struct vtm_http_client_res *res);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_CLIENT_H_ */
