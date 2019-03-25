/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "http_server.h"

#include <string.h> /* memset() */
#include <vtm/core/error.h>
#include <vtm/core/lang.h>
#include <vtm/net/socket_stream_server.h>
#include <vtm/net/http/http_connection_intl.h>
#include <vtm/net/http/http_connection_base_intl.h>
#include <vtm/net/http/http_request.h>
#include <vtm/net/http/http_request_intl.h>
#include <vtm/net/http/http_response_intl.h>
#include <vtm/net/http/ws_connection_intl.h>
#include <vtm/net/http/ws_message_intl.h>
#include <vtm/util/spinlock.h>

#define VTM_HTTP_WD_RESPONSE          "_RESPONSE"

struct vtm_http_srv
{
	vtm_socket_stream_srv     *sock_srv;
	struct vtm_http_srv_opts  *opts;
	struct vtm_http_srv_cbs   cbs;
	vtm_http_mem              *mem;
	struct vtm_spinlock       stop_lock;
};

/* forward declaration */
static void vtm_http_srv_init_callbacks(struct vtm_socket_stream_srv_cbs *cbs);
static enum vtm_socket_family vtm_http_srv_determine_sock_family(const char *addr);
static VTM_INLINE void vtm_http_srv_fill_ctx(vtm_http_srv *srv, struct vtm_http_ctx *ctx, vtm_dataset *wd);

/* http connection */
static bool vtm_http_srv_http_handle_request(vtm_http_srv *srv, vtm_dataset *wd, struct vtm_http_con_base *bcon);
static void vtm_http_srv_http_con_upgrade_ws(vtm_http_srv *srv, vtm_dataset *wd, vtm_http_con *con, vtm_http_res *res);

/* ws connection */
static bool vtm_http_srv_ws_handle_request(vtm_http_srv *srv, vtm_dataset *wd, struct vtm_http_con_base *bcon);

/* forward declaration callbacks */
static void vtm_http_srv_server_ready(vtm_socket_stream_srv *sock_srv, struct vtm_socket_stream_srv_opts *opts);
static void vtm_http_srv_worker_init(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd);
static void vtm_http_srv_worker_end(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd);
static void vtm_http_srv_sock_connected(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *sock);
static void vtm_http_srv_sock_closed(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *sock);
static void vtm_http_srv_sock_can_read(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *sock);
static void vtm_http_srv_sock_can_write(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *sock);
static void vtm_http_srv_sock_error(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *sock);
static void vtm_http_srv_sock_unregister(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *sock);

vtm_http_srv* vtm_http_srv_new(void)
{
	vtm_http_srv *srv = malloc(sizeof(vtm_http_srv));
	if (!srv) {
		vtm_err_oom();
		return NULL;
	}

	memset(srv, 0, sizeof(vtm_http_srv));
	vtm_spinlock_init(&srv->stop_lock);

	return srv;
}

void vtm_http_srv_free(vtm_http_srv *srv)
{
	if (!srv)
		return;

	free(srv);
}

int vtm_http_srv_run(vtm_http_srv *srv, struct vtm_http_srv_opts *opts)
{
	int rc;
	struct vtm_socket_stream_srv_opts stream_opts;

	if (!opts)
		return vtm_err_set(VTM_E_INVALID_ARG);

	/* create stream server */
	vtm_spinlock_lock(&srv->stop_lock);
	srv->sock_srv = vtm_socket_stream_srv_new();
	if (!srv->sock_srv) {
		rc = vtm_err_get_code();
		goto unlock;
	}

	/* set callbacks */
	srv->opts = opts;
	srv->cbs = opts->cbs;

	/* prepare stream server options */
	memset(&stream_opts.cbs, 0, sizeof(stream_opts.cbs));
	vtm_http_srv_init_callbacks(&stream_opts.cbs);

	stream_opts.addr.family = vtm_http_srv_determine_sock_family(opts->host);
	stream_opts.addr.host = opts->host;
	stream_opts.addr.port = opts->port;
	stream_opts.tls = opts->tls;
	stream_opts.backlog = opts->backlog;
	stream_opts.events = opts->events;
	stream_opts.threads = opts->threads;

	/* run stream server */
	vtm_socket_stream_srv_set_usr_data(srv->sock_srv, srv);
	rc = vtm_socket_stream_srv_run(srv->sock_srv, &stream_opts);

	/* if run was successful, stop_lock was unlocked in server_ready callback */
	if (rc == VTM_OK)
		vtm_spinlock_lock(&srv->stop_lock);

	/* free stream server */
	vtm_socket_stream_srv_free(srv->sock_srv);
	srv->sock_srv = NULL;

unlock:
	vtm_spinlock_unlock(&srv->stop_lock);

	return rc;
}

int vtm_http_srv_stop(vtm_http_srv *srv)
{
	int rc;

	vtm_spinlock_lock(&srv->stop_lock);

	if (srv)
		rc = vtm_socket_stream_srv_stop(srv->sock_srv);
	else
		rc = VTM_E_INVALID_STATE;

	vtm_spinlock_unlock(&srv->stop_lock);

	return rc;
}

static enum vtm_socket_family vtm_http_srv_determine_sock_family(const char *addr)
{
	const char *p;

	for (p = addr; *p != '\0'; p++) {
		switch (*p) {
			case '.':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				continue;

			default:
				return VTM_SOCK_FAM_IN6;
		}
	}

	return VTM_SOCK_FAM_IN4;
}

static int vtm_http_srv_con_create(vtm_http_srv *srv, vtm_socket *sock)
{
	vtm_http_con *con;

	con = vtm_http_con_new(sock);
	if (!con)
		return VTM_ERROR;

	((struct vtm_http_con_base*) con)->con_handle_req = vtm_http_srv_http_handle_request;
	vtm_socket_set_usr_data(sock, con);

	return VTM_OK;
}

static int vtm_http_srv_ws_con_create(vtm_http_srv *srv, vtm_socket *sock, vtm_ws_con **con)
{
	*con = vtm_ws_con_new(VTM_WS_MODE_SERVER, sock);
	if (!*con)
		return vtm_err_get_code();

	((struct vtm_http_con_base*) *con)->con_handle_req = vtm_http_srv_ws_handle_request;
	vtm_socket_set_usr_data(sock, *con);

	return VTM_OK;
}

static void vtm_http_srv_init_callbacks(struct vtm_socket_stream_srv_cbs *cbs)
{
	cbs->server_ready = vtm_http_srv_server_ready;

	cbs->worker_init = vtm_http_srv_worker_init;
	cbs->worker_end = vtm_http_srv_worker_end;

	cbs->sock_connected = vtm_http_srv_sock_connected;
	cbs->sock_disconnected = vtm_http_srv_sock_closed;
	cbs->sock_can_read = vtm_http_srv_sock_can_read;
	cbs->sock_can_write = vtm_http_srv_sock_can_write;
	cbs->sock_error = vtm_http_srv_sock_error;
}

static void vtm_http_srv_server_ready(vtm_socket_stream_srv *sock_srv, struct vtm_socket_stream_srv_opts *opts)
{
	vtm_http_srv *srv;

	srv = vtm_socket_stream_srv_get_usr_data(sock_srv);
	VTM_ASSERT(srv);

	vtm_spinlock_unlock(&srv->stop_lock);

	if (srv->cbs.server_ready)
		srv->cbs.server_ready(srv, srv->opts);
}

static void vtm_http_srv_worker_init(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd)
{
	vtm_http_srv *srv;
	vtm_http_res *res;
	struct vtm_http_ctx ctx;

	res = vtm_http_res_new();
	vtm_dataset_set_pointer(wd, VTM_HTTP_WD_RESPONSE, res);

	srv = vtm_socket_stream_srv_get_usr_data(sock_srv);
	VTM_ASSERT(srv);

	if (srv->cbs.worker_init) {
		vtm_http_srv_fill_ctx(srv, &ctx, wd);
		srv->cbs.worker_init(&ctx);
	}
}

static void vtm_http_srv_worker_end(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd)
{
	vtm_http_srv *srv;
	vtm_http_res *res;
	struct vtm_http_ctx ctx;

	res = vtm_dataset_get_pointer(wd, VTM_HTTP_WD_RESPONSE);
	vtm_http_res_free(res);

	srv = vtm_socket_stream_srv_get_usr_data(sock_srv);
	VTM_ASSERT(srv);

	if (srv->cbs.worker_end) {
		vtm_http_srv_fill_ctx(srv, &ctx, wd);
		srv->cbs.worker_end(&ctx);
	}
}

static void vtm_http_srv_sock_connected(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *sock)
{
	vtm_http_srv *srv;

	vtm_socket_set_opt(sock, VTM_SOCK_OPT_TCP_NODELAY, (bool[]) {true}, sizeof(bool));
	vtm_socket_set_state(sock, VTM_SOCK_STAT_NBL_AUTO | VTM_SOCK_STAT_NBL_READ);

	srv = vtm_socket_stream_srv_get_usr_data(sock_srv);
	vtm_http_srv_con_create(srv, sock);
}

static void vtm_http_srv_sock_can_read(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *sock)
{
	vtm_http_srv *srv;
	struct vtm_http_con_base *con;
	enum vtm_net_recv_stat stat;
	bool loop;

	srv = vtm_socket_stream_srv_get_usr_data(sock_srv);
	VTM_ASSERT(srv);

	con = vtm_socket_get_usr_data(sock);
	VTM_ASSERT(con);

	loop = true;
	while (loop) {
		stat =  con->con_can_read(con);
		switch (stat) {
			case VTM_NET_RECV_STAT_ERROR:
			case VTM_NET_RECV_STAT_INVALID:
			case VTM_NET_RECV_STAT_CLOSED:
				vtm_socket_close(sock);
				return;

			case VTM_NET_RECV_STAT_AGAIN:
				return;

			case VTM_NET_RECV_STAT_COMPLETE:
				loop = con->con_handle_req(srv, wd, con);
				break;
		}
	}
}

static void vtm_http_srv_sock_can_write(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *sock)
{
	int rc;
	struct vtm_http_con_base *con;

	con = vtm_socket_get_usr_data(sock);
	VTM_ASSERT(con);

	rc = con->con_can_write(con);
	if (rc != VTM_OK && rc != VTM_E_IO_AGAIN)
		vtm_socket_close(sock);
}

static void vtm_http_srv_sock_closed(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *sock)
{
	vtm_http_srv_sock_unregister(sock_srv, wd, sock);
}

static void vtm_http_srv_sock_error(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *sock)
{
}

static void vtm_http_srv_sock_unregister(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *sock)
{
	vtm_http_srv *srv;
	struct vtm_http_con_base *con;
	struct vtm_http_ctx ctx;

	srv = vtm_socket_stream_srv_get_usr_data(sock_srv);
	VTM_ASSERT(srv);

	con = vtm_socket_get_usr_data(sock);
	if (!con)
		return;

	switch (con->type) {
		case VTM_HTTP_CON_TYPE_H1:
			vtm_http_con_free((vtm_http_con*) con);
			break;

		case VTM_HTTP_CON_TYPE_WS:
			if (srv->cbs.ws_close) {
				vtm_http_srv_fill_ctx(srv, &ctx, wd);
				srv->cbs.ws_close(&ctx, (vtm_ws_con*) con);
			}
			vtm_ws_con_free((vtm_ws_con*) con);
			break;
	}
}

static bool vtm_http_srv_http_handle_request(vtm_http_srv *srv, vtm_dataset *wd, struct vtm_http_con_base *bcon)
{
	int rc;
	vtm_http_con *con;
	struct vtm_http_ctx ctx;
	struct vtm_http_req req;
	vtm_http_res *res;
	enum vtm_http_res_act act;

	con = (vtm_http_con*) bcon;
	rc = vtm_http_con_get_request(con, &req);
	if (rc != VTM_OK)
		return false;

	res = vtm_dataset_get_pointer(wd, VTM_HTTP_WD_RESPONSE);
	vtm_http_res_prepare(res, &req);

	vtm_http_srv_fill_ctx(srv, &ctx, wd);
	srv->cbs.http_request(&ctx, &req, res);
	vtm_http_req_release(&req);

	act = vtm_http_res_get_action(res);
	if (!vtm_http_res_was_sent(res))
		act = VTM_HTTP_RES_ACT_CLOSE_CON;

	switch (act) {
		case VTM_HTTP_RES_ACT_CLOSE_CON:
			vtm_socket_close(vtm_http_con_get_socket(con));
			return false;

		case VTM_HTTP_RES_ACT_KEEP_CON:
			return true;

		case VTM_HTTP_RES_ACT_UPGRADE_WS:
			vtm_http_srv_http_con_upgrade_ws(srv, wd, con, res);
			return false;
	}

	return false;
}

static void vtm_http_srv_http_con_upgrade_ws(vtm_http_srv *srv, vtm_dataset *wd, vtm_http_con *con, vtm_http_res *res)
{
	int rc;
	vtm_socket *sock;
	vtm_ws_con *ws_con;
	struct vtm_http_ctx ctx;

	sock = vtm_http_con_get_socket(con);

	vtm_http_con_free(con);

	rc = vtm_http_srv_ws_con_create(srv, sock, &ws_con);
	if (rc != VTM_OK) {
		vtm_socket_set_usr_data(sock, NULL);
		vtm_socket_close(sock);
		return;
	}

	if (srv->cbs.ws_connect) {
		vtm_http_srv_fill_ctx(srv, &ctx, wd);
		srv->cbs.ws_connect(&ctx, ws_con);
	}
}

static bool vtm_http_srv_ws_handle_request(vtm_http_srv *srv, vtm_dataset *wd, struct vtm_http_con_base *bcon)
{
	int rc;
	vtm_ws_con *con;
	struct vtm_ws_msg msg;
	struct vtm_http_ctx ctx;

	con = (vtm_ws_con*) bcon;
	rc = vtm_ws_con_get_msg(con, &msg);
	if (rc != VTM_OK)
		return false;

	if (srv->cbs.ws_message)
		srv->cbs.ws_message(&ctx, &msg);

	vtm_ws_msg_release(&msg);

	return true;
}

static VTM_INLINE void vtm_http_srv_fill_ctx(vtm_http_srv *srv, struct vtm_http_ctx *ctx, vtm_dataset *wd)
{
	ctx->mem = srv->mem;
	ctx->wd = wd;
}
