/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file ws_message.h
 *
 * @brief WebSocket message
 */

#ifndef VTM_NET_HTTP_WS_MESSAGE_H_
#define VTM_NET_HTTP_WS_MESSAGE_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/net/http/ws.h>
#include <vtm/net/http/ws_connection.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vtm_ws_msg
{
	vtm_ws_con            *con;   /**< connection from which the message was received */
	enum vtm_ws_msg_type   type;  /**< type of the message */
	void                  *data;  /**< pointer to payload of the message */
	size_t                 len;   /**< payload length in bytes */
};

/**
 * Releases all allocated resources of the message.
 *
 * @param msg the message that should be released.
 */
VTM_API void vtm_ws_msg_release(struct vtm_ws_msg *msg);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_WS_MESSAGE_H_ */
