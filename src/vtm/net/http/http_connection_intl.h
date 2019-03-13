/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_NET_HTTP_HTTP_CONNECTION_INTL_H_
#define VTM_NET_HTTP_HTTP_CONNECTION_INTL_H_

#include <vtm/net/common.h>
#include <vtm/net/socket.h>
#include <vtm/net/socket_emitter.h>
#include <vtm/net/http/http_server.h>
#include <vtm/net/http/http_request.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtm_http_con vtm_http_con;

vtm_http_con* vtm_http_con_new(vtm_socket *sock);
void vtm_http_con_free(vtm_http_con *con);

vtm_socket* vtm_http_con_get_socket(vtm_http_con *con);
int vtm_http_con_get_request(vtm_http_con *con, struct vtm_http_req *req);

void vtm_http_con_set_emitter(vtm_http_con *con, struct vtm_socket_emitter *se);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_CONNECTION_INTL_H_ */
