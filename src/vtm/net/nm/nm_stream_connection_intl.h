/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_NET_NM_NM_STREAM_CONNECTION_INTL_H_
#define VTM_NET_NM_NM_STREAM_CONNECTION_INTL_H_

#include <vtm/core/dataset.h>
#include <vtm/net/common.h>
#include <vtm/net/socket.h>
#include <vtm/net/nm/nm_stream_connection.h>

#ifdef __cplusplus
extern "C" {
#endif

vtm_nm_stream_con* vtm_nm_stream_con_new(vtm_socket *sock);
void vtm_nm_stream_con_free(vtm_nm_stream_con *con);

enum vtm_net_recv_stat vtm_nm_stream_con_read(vtm_nm_stream_con *con);
int vtm_nm_stream_con_write(vtm_nm_stream_con *con);

vtm_dataset* vtm_nm_stream_con_get_msg(vtm_nm_stream_con *con);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_NM_NM_STREAM_CONNECTION_INTL_H_ */
