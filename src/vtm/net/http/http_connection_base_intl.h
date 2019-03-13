/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_NET_HTTP_HTTP_CONNECTION_BASE_INTL_H_
#define VTM_NET_HTTP_HTTP_CONNECTION_BASE_INTL_H_

#include <vtm/net/common.h>
#include <vtm/net/socket.h>
#include <vtm/net/http/http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

enum vtm_http_con_type
{
	VTM_HTTP_CON_TYPE_H1,
	VTM_HTTP_CON_TYPE_WS
};

struct vtm_http_con_base
{
	vtm_socket                  *sock;
	enum vtm_http_con_type      type;

	enum vtm_net_recv_stat (*con_can_read)(struct vtm_http_con_base *con);
	int (*con_can_write)(struct vtm_http_con_base *con);
	bool (*con_handle_req)(vtm_http_srv *srv, vtm_dataset *wd, struct vtm_http_con_base *con);
};

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_CONNECTION_BASE_INTL_H_ */
