/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_NET_HTTP_WS_CONNECTION_INTL_H_
#define VTM_NET_HTTP_WS_CONNECTION_INTL_H_

#include <vtm/core/types.h> /* size_t */
#include <vtm/net/common.h>
#include <vtm/net/socket_emitter.h>
#include <vtm/net/http/ws.h>
#include <vtm/net/http/ws_connection.h>
#include <vtm/net/http/ws_message.h>

#ifdef __cplusplus
extern "C" {
#endif

vtm_ws_con* vtm_ws_con_new(enum vtm_ws_mode mode, vtm_socket *sock);
void vtm_ws_con_free(vtm_ws_con *con);

int vtm_ws_con_get_msg(vtm_ws_con *con, struct vtm_ws_msg *msg);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_WS_CONNECTION_INTL_H_ */
