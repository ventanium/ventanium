/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file http_upgrade.h
 *
 * @brief Protocol upgrade helper
 */

#ifndef VTM_NET_HTTP_HTTP_UPGRADE_H_
#define VTM_NET_HTTP_HTTP_UPGRADE_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/net/http/http_request.h>
#include <vtm/net/http/http_response.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Checks if the given request is a WebSocket handshake request.
 *
 * @param req the request that should be checked
 * @return true if request is a WebSocket handshake request
 * @return false if not a handshake request
 */
VTM_API bool vtm_http_is_ws_request(struct vtm_http_req *req);

/**
 * Extracts the requested WebSocket protocols from the request.
 *
 * @param req the request where the protocols should be extracted from
 * @param[out] protos will be filled with array of pointers to protocol names
 * @param[out] proto_count will be filled with the number of protocol names
 */
VTM_API void vtm_http_get_ws_protocols(struct vtm_http_req *req, char ***protos, size_t *proto_count);

/**
 * Prepares the response so that the connection is upgraded
 * to WebSocket protocol afterwards.
 *
 * @param req the WebSocket handshake request
 * @param res the response that should be filled
 * @param proto the chosen WebSocket protocol, can be NULL
 * @return VTM_OK if the response was successfully prepared
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_http_upgrade_to_ws(struct vtm_http_req *req, vtm_http_res *res, char *proto);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_UPGRADE_H_ */
