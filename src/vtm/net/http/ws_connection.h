/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file ws_connection.h
 *
 * @brief WebSocket server-side connection
 */

#ifndef VTM_NET_HTTP_WS_CONNECTION_H_
#define VTM_NET_HTTP_WS_CONNECTION_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/net/http/ws.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtm_ws_con vtm_ws_con;

/**
 * Sends a message to the peer.
 *
 * @param con the connection where the message should be sent over
 * @param type the type of the WebSocket message
 * @param data pointer to message payload
 * @param len length of payload in bytes
 * @return VTM_OK if the transmission was successfully started
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occcured
 */
VTM_API int vtm_ws_con_send_msg(vtm_ws_con *con, enum vtm_ws_msg_type type, const void *data, size_t len);

/**
 * Retrieves the source ip address and used port of a client connection.
 *
 * @param con the connection whose remote address should be retrieved
 * @param[out] buf the buffer where the ip address string representation is
 *             stored
 * @param len the length of the buffer in bytes
 * @param[out] port the used port number
 * @return VTM_OK if the address was successfully retrieved
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occcured
 */
VTM_API int vtm_ws_con_get_remote_info(vtm_ws_con *con, char *buf, size_t len, unsigned int *port);

#endif /* VTM_NET_HTTP_WS_CONNECTION_H_ */
