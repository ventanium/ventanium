/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file http_response.h
 *
 * @brief HTTP response
 */

#ifndef VTM_NET_HTTP_HTTP_RESPONSE_H_
#define VTM_NET_HTTP_HTTP_RESPONSE_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/net/socket_emitter.h>
#include <vtm/net/http/http.h>

#ifdef __cplusplus
extern "C" {
#endif

enum vtm_http_res_mode
{
	VTM_HTTP_RES_MODE_FIXED,     /**< Response body is buffered and then sent as whole */
	VTM_HTTP_RES_MODE_CHUNKED    /**< Response body is sent in chunks without buffering */
};

enum vtm_http_res_act
{
	VTM_HTTP_RES_ACT_CLOSE_CON,  /**< close connection after response has been sent */
	VTM_HTTP_RES_ACT_KEEP_CON,   /**< keep connection alive */
	VTM_HTTP_RES_ACT_UPGRADE_WS  /**< Upgrade connection to a WebSocket connection */
};

typedef struct vtm_http_res vtm_http_res;

/**
 * Initializes and starts the response with mode and status code.
 *
 * This must always be the first call on a response handle.
 *
 * @param res the response
 * @param mode the mode how the body is sent
 * @param status one of the three digit HTTP status codes
 * @return VTM_OK if the call succeeded
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_http_res_begin(vtm_http_res *res, enum vtm_http_res_mode mode, int status);

/**
 * Sets a response header.
 *
 * @param res the response
 * @param name the name of the header field
 * @param value the value for the header field
 * @return VTM_OK if the call succeeded
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_http_res_header(vtm_http_res *res, const char *name, const char *value);

/**
 * Adds a string to the response body.
 *
 * @param res the response
 * @param data the string that should be appended to the body
 * @return VTM_OK if the call succeeded
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_http_res_body_str(vtm_http_res *res, const char *data);

/**
 * Adds a chunk of memory to the response body
 *
 * @param res the response
 * @param src pointer to the memory chunk
 * @param len length of memory region in bytes
 * @return VTM_OK if the call succeeded
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_http_res_body_raw(vtm_http_res *res, const char *src, size_t len);

/**
 * Lets the response body be defined by a socket emitter.
 *
 * The emitter is automatically released.
 *
 * @param res the response
 * @param se pointer to the socket emitter
 * @return VTM_OK if the call succeeded
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_http_res_body_emt(vtm_http_res *res, struct vtm_socket_emitter *se);

/**
 * Sets the action that is performed after the response has been sent.
 *
 * This is used for example when upgrading an HTTP connection to a
 * WebSocket connection.
 *
 * The action can only be changed before the body was written.
 *
 * @param res the response
 * @param act the action that should be performed
 * @param data additional data needed by the action
 * @return VTM_OK if the call succeeded
 * @return VTM_E_INVALID_STATE if the response body was already started
 */
VTM_API int vtm_http_res_set_action(vtm_http_res *res, enum vtm_http_res_act act, void *data);

/**
 * Marks the response as complete and starts sending.
 *
 * @param res the response that is complete
 * @return VTM_OK if the call succeeded
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_http_res_end(vtm_http_res *res);

/**
 * Sets HTTP date header.
 *
 * @param res the response
 * @return VTM_OK if the call succeeded
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_http_res_set_date(vtm_http_res *res);

/**
 * Checks if the reponse was already started.
 *
 * @param res the response that should be checked
 * @return true if the response was started, false otherwise
 */
VTM_API bool vtm_http_res_was_started(vtm_http_res *res);

/**
 * Checks if the response was sent by vtm_http_res_end().
 *
 * @param res the response that should be checked
 * @return true if the response was sent, false otherwise
 */
VTM_API bool vtm_http_res_was_sent(vtm_http_res *res);

/**
 * Gets the HTTP version of the response
 *
 * @pram res the response whose version should be retrieved
 * @return the HTTP version of the response
 */
VTM_API enum vtm_http_version vtm_http_res_get_version(vtm_http_res *res);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_RESPONSE_H_ */
