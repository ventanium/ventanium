/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "ws_connection.h"

#include <string.h> /* memset() */
#include <vtm/core/error.h>
#include <vtm/net/socket_connection.h>
#include <vtm/net/http/http_connection_base_intl.h>
#include <vtm/net/http/ws_parser.h>
#include <vtm/net/http/ws_frame_intl.h>
#include <vtm/net/http/ws_connection_intl.h>
#include <vtm/net/http/ws_message_intl.h>

struct vtm_ws_con
{
	struct vtm_http_con_base   base;
	struct vtm_socket_con      sock_con;
	struct vtm_ws_parser       parser;
};

/* forward declaration */
static int vtm_ws_con_write(struct vtm_http_con_base *base_con);
static enum vtm_net_recv_stat vtm_ws_con_read(struct vtm_http_con_base *base_con);

vtm_ws_con* vtm_ws_con_new(enum vtm_ws_mode mode, vtm_socket *sock)
{
	vtm_ws_con *con;

	con = malloc(sizeof(vtm_ws_con));
	if (!con) {
		vtm_err_oom();
		return NULL;
	}

	if (vtm_socket_con_init(&con->sock_con, sock) != VTM_OK) {
		free(con);
		return NULL;
	}

	if (vtm_ws_parser_init(&con->parser, mode) != VTM_OK) {
		free(con);
		return NULL;
	}

	con->base.sock = sock;
	con->base.type = VTM_HTTP_CON_TYPE_WS;
	con->base.con_can_read = vtm_ws_con_read;
	con->base.con_can_write = vtm_ws_con_write;
	con->base.con_handle_req = NULL;

	return con;
}

void vtm_ws_con_free(vtm_ws_con *con)
{
	vtm_socket_con_release(&con->sock_con);
	vtm_ws_parser_release(&con->parser);
	free(con);
}

static enum vtm_net_recv_stat vtm_ws_con_read(struct vtm_http_con_base *base_con)
{
	int rc;
	vtm_ws_con *con;
	size_t read;

	con = (vtm_ws_con*) base_con;

	/* make space in buffer */
	rc = vtm_buf_ensure(&con->sock_con.recvbuf, 512);
	if (rc != VTM_OK)
		return VTM_NET_RECV_STAT_ERROR;

	/* read from socket to buffer */
	rc = vtm_socket_read(con->sock_con.sock,
		VTM_BUF_PUT_PTR(&con->sock_con.recvbuf),
		VTM_BUF_PUT_AVAIL_TOTAL(&con->sock_con.recvbuf), &read);

	VTM_BUF_PUT_INC(&con->sock_con.recvbuf, read);
	if (rc != VTM_OK && rc != VTM_E_IO_AGAIN)
		return VTM_NET_RECV_STAT_ERROR;

	/* run parser */
	return vtm_ws_parser_run(&con->parser, &con->sock_con.recvbuf);
}

static int vtm_ws_con_write(struct vtm_http_con_base *base_con)
{
	vtm_ws_con *con;

	con = (vtm_ws_con*) base_con;

	return vtm_socket_con_write(&con->sock_con);
}

int vtm_ws_con_get_msg(vtm_ws_con *con, struct vtm_ws_msg *msg)
{
	int rc;

	rc = vtm_ws_parser_get_msg(&con->parser, msg);
	msg->con = con;

	return rc;
}

int vtm_ws_con_send_msg(vtm_ws_con *con, enum vtm_ws_msg_type type, const void *data, size_t len)
{
	int rc;
	struct vtm_ws_frame_desc desc;

	memset(&desc, 0, sizeof(desc));
	desc.fin = true;
	desc.opcode = type;
	desc.len = len;

	vtm_socket_con_write_lock(&con->sock_con);

	rc = vtm_ws_frame_write_header(&con->sock_con.sendbuf, &desc);
	if (rc != VTM_OK)
		goto end;

	rc = vtm_buf_putm(&con->sock_con.sendbuf, data, len);
	if (rc != VTM_OK)
		goto end;

	rc = vtm_socket_con_write_start(&con->sock_con);
	if (rc == VTM_E_IO_AGAIN)
		rc = VTM_OK;

end:
	vtm_socket_con_write_unlock(&con->sock_con);

	return rc;
}

int vtm_ws_con_get_remote_info(vtm_ws_con *con, char *buf, size_t len, unsigned int *port)
{
	int rc;
	struct vtm_socket_saddr saddr;

	rc = vtm_socket_get_remote_addr(con->sock_con.sock, &saddr);
	if (rc != VTM_OK)
		return rc;

	return vtm_socket_os_addr_convert(&saddr, NULL, buf, len, port);
}
