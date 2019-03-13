/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "nm_stream_connection_intl.h"

#include <vtm/core/buffer.h>
#include <vtm/core/error.h>
#include <vtm/net/common.h>
#include <vtm/net/socket_connection.h>
#include <vtm/net/nm/nm_parser_intl.h>
#include <vtm/net/nm/nm_protocol_intl.h>

struct vtm_nm_stream_con
{
	struct vtm_socket_con  sock_con;
	struct vtm_nm_parser   parser;
};

vtm_nm_stream_con* vtm_nm_stream_con_new(vtm_socket *sock)
{
	vtm_nm_stream_con *con;

	con = malloc(sizeof(*con));
	if (!con) {
		vtm_err_oom();
		return NULL;
	}

	if (vtm_socket_con_init(&con->sock_con, sock) != VTM_OK) {
		free(con);
		return NULL;
	}

	vtm_nm_parser_init(&con->parser);

	return con;
}

void vtm_nm_stream_con_free(vtm_nm_stream_con *con)
{
	if (!con)
		return;

	vtm_socket_con_release(&con->sock_con);
	vtm_nm_parser_release(&con->parser);

	free(con);
}

void vtm_nm_stream_con_set_usr_data(vtm_nm_stream_con *con, void *data)
{
	vtm_socket_con_set_usr_data(&con->sock_con, data);
}

void* vtm_nm_stream_con_get_usr_data(vtm_nm_stream_con *con)
{
	return vtm_socket_con_get_usr_data(&con->sock_con);
}

int vtm_nm_stream_con_send(vtm_nm_stream_con *con, vtm_dataset *msg)
{
	int rc;

	vtm_socket_con_write_lock(&con->sock_con);

	rc = vtm_nm_msg_to_buf(msg, &con->sock_con.sendbuf);
	if (rc == VTM_OK)
		rc = vtm_socket_con_write_start(&con->sock_con);

	vtm_socket_con_write_unlock(&con->sock_con);

	if (rc == VTM_E_IO_AGAIN)
		rc = VTM_OK;

	return rc;
}

void vtm_nm_stream_con_close(vtm_nm_stream_con *con)
{
	vtm_socket_close(con->sock_con.sock);
}

enum vtm_net_recv_stat vtm_nm_stream_con_read(vtm_nm_stream_con *con)
{
	int rc;
	size_t read;

	rc = vtm_buf_ensure(&con->sock_con.recvbuf, 512);
	if (rc != VTM_OK)
		return VTM_NET_RECV_STAT_ERROR;

	rc = vtm_socket_read(con->sock_con.sock, VTM_BUF_PUT_PTR(&con->sock_con.recvbuf),
		VTM_BUF_PUT_AVAIL_TOTAL(&con->sock_con.recvbuf), &read);

	VTM_BUF_PUT_INC(&con->sock_con.recvbuf, read);
	switch (rc) {
		case VTM_OK:
			return vtm_nm_parser_run(&con->parser, &con->sock_con.recvbuf);

		case VTM_E_IO_AGAIN:
			return VTM_NET_RECV_STAT_AGAIN;

		default:
			break;
	}
	return VTM_NET_RECV_STAT_ERROR;
}

int vtm_nm_stream_con_write(vtm_nm_stream_con *con)
{
	return vtm_socket_con_write(&con->sock_con);
}

vtm_dataset* vtm_nm_stream_con_get_msg(vtm_nm_stream_con *con)
{
	vtm_dataset *msg;

	msg = vtm_nm_parser_get_msg(&con->parser);
	vtm_buf_discard_processed(&con->sock_con.recvbuf);

	return msg;
}
