/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "http_connection_intl.h"

#include <vtm/core/buffer.h>
#include <vtm/core/error.h>
#include <vtm/net/http/http_connection_base_intl.h>
#include <vtm/net/http/http_parser.h>

struct vtm_http_con
{
	struct vtm_http_con_base     base;
	struct vtm_buf               recvbuf;
	struct vtm_http_parser       parser;
	struct vtm_socket_emitter    *emitter;
	bool                         clear;
};

/* forward declaration */
static enum vtm_net_recv_stat vtm_http_con_read(struct vtm_http_con_base *base_con);
static int vtm_http_con_write(struct vtm_http_con_base *base_con);

vtm_http_con* vtm_http_con_new(vtm_socket *sock)
{
	vtm_http_con *con;

	con = malloc(sizeof(vtm_http_con));
	if (!con) {
		vtm_err_oom();
		return NULL;
	}

	con->base.sock = sock;
	con->base.type = VTM_HTTP_CON_TYPE_H1;

	con->base.con_can_read = vtm_http_con_read;
	con->base.con_can_write = vtm_http_con_write;
	con->base.con_handle_req = NULL;

	con->emitter = NULL;
	con->clear = false;

	vtm_buf_init(&con->recvbuf, VTM_BYTEORDER_LE);
	vtm_http_parser_init(&con->parser, VTM_HTTP_PM_REQUEST);

	return con;
}

void vtm_http_con_free(vtm_http_con *con)
{
	vtm_buf_release(&con->recvbuf);
	vtm_http_parser_release(&con->parser);
	free(con);
}

vtm_socket* vtm_http_con_get_socket(vtm_http_con *con)
{
	return con->base.sock;
}

int vtm_http_con_get_request(vtm_http_con *con, struct vtm_http_req *req)
{
	if (con->parser.state != VTM_HTTP_PARSE_COMPLETE)
		return VTM_ERROR;

	req->method = con->parser.req_method;
	req->version = con->parser.version;
	req->path = con->parser.req_path;
	req->headers = con->parser.headers;
	req->params = con->parser.req_params;
	req->con = con;

	con->clear = true;
	vtm_http_parser_reset(&con->parser);

	return VTM_OK;
}

void vtm_http_con_set_emitter(vtm_http_con *con, struct vtm_socket_emitter *se)
{
	con->emitter = se;
}

static enum vtm_net_recv_stat vtm_http_con_read(struct vtm_http_con_base *base_con)
{
	int rc;
	vtm_http_con *con;
	size_t read;

	con = (vtm_http_con*) base_con;

	/* discard buffer contents from previous request */
	if (con->clear) {
		con->clear = false;
		vtm_buf_discard_processed(&con->recvbuf);
	}

	/* make space in buffer */
	rc = vtm_buf_ensure(&con->recvbuf, 512);
	if (rc != VTM_OK)
		return VTM_NET_RECV_STAT_ERROR;

	/* read from socket to buffer */
	rc = vtm_socket_read(con->base.sock, VTM_BUF_PUT_PTR(&con->recvbuf),
			VTM_BUF_PUT_AVAIL_TOTAL(&con->recvbuf), &read);

	VTM_BUF_PUT_INC(&con->recvbuf, read);
	if (rc != VTM_OK && rc != VTM_E_IO_AGAIN)
		return VTM_NET_RECV_STAT_ERROR;

	/* run parser */
	return vtm_http_parser_run(&con->parser, &con->recvbuf);
}

static int vtm_http_con_write(struct vtm_http_con_base *base_con)
{
	int rc;
	struct vtm_http_con *con;
	struct vtm_socket_emitter *se;

	con = (struct vtm_http_con*) base_con;
	se = con->emitter;
	VTM_ASSERT(se);

	rc = vtm_socket_emitter_try_write(&se);
	if (rc != VTM_OK && rc != VTM_E_IO_AGAIN) {
		vtm_socket_emitter_free_chain(se);
		se = NULL;
	}

	con->emitter = se;

	return rc;
}
