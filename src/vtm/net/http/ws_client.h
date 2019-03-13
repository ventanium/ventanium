/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file ws_client.h
 *
 * @brief WebSocket client
 */

#ifndef VTM_NET_HTTP_WS_CLIENT_H_
#define VTM_NET_HTTP_WS_CLIENT_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/net/network.h>
#include <vtm/net/http/ws.h>
#include <vtm/net/http/ws_message.h>
#include <vtm/net/socket_spec.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VTM_WS_CL_OPT_NO_CERT_CHECK       1  /**< expects bool */
#define VTM_WS_CL_OPT_TIMEOUT             2  /**< expects unsigned long, value is millisceonds */

typedef struct vtm_ws_client vtm_ws_client;

/**
 * Creates a new client.
 *
 * @return the created client which can be used in the other functions
 * @return NULL if an error occured
 */
VTM_API vtm_ws_client* vtm_ws_client_new(void);

/**
 * Releases the client and all allocated resources.
 *
 * After this call the client pointer is no longer valid.
 *
 * @param cl the client that should be released
 */
VTM_API void vtm_ws_client_free(vtm_ws_client *cl);

/**
 * Sets one of the possible options.
 *
 * The possible options are macros starting with VTM_WS_CL_OPT_.
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
VTM_API int vtm_ws_client_set_opt(vtm_ws_client *cl, int opt, const void *val, size_t len);

/**
 * Connects to given HTTP/WebSocket server
 *
 * @param cl the client
 * @param fam the desired socket family (IPv4 or IPv6)
 * @param url the url where to connect to (must use http/https as scheme)
 * @return VTM_OK if the client connected to the server
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_ws_client_connect(vtm_ws_client *cl, enum vtm_socket_family fam, const char *url);

/**
 * Closes the connection to the server.
 *
 * @param cl the client that should close the connection
 * @return VTM_OK if the connection was closed successfully
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_ws_client_close(vtm_ws_client *cl);

/**
 * Sends a message to the server.
 *
 * @param cl the client
 * @param type the type of the message
 * @param src pointer to payload of message
 * @param len length of payload
 * @return VTM_OK if the message was successfully sent
 * @return VTM_E_INVALID_STATE if client is not connected to a server
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_ws_client_send(vtm_ws_client *cl, enum vtm_ws_msg_type type, const void *src, size_t len);

/**
 * Tries to receive a message.
 *
 * @param cl the client
 * @param[out] msg the structure where the received message is stored
 * @return VTM_OK if a message was successfully received
 * @return VTM_E_INVALID_STATE if client is not connected to a server
 * @return VTM_E_TIMEOUT if the timeout elapsed and no message was received
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_ws_client_recv(vtm_ws_client *cl, struct vtm_ws_msg *msg);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_WS_CLIENT_H_ */
