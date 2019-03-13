/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "nm_stream_server.h"

#include <string.h> /* memset() */
#include <vtm/core/error.h>
#include <vtm/net/socket_stream_server.h>
#include <vtm/net/nm/nm_stream_connection_intl.h>
#include <vtm/util/spinlock.h>

struct vtm_nm_stream_srv
{
	vtm_socket_stream_srv          *sock_srv;
	struct vtm_nm_stream_srv_opts  *opts;
	struct vtm_nm_stream_srv_cbs   cbs;
	struct vtm_spinlock            stop_lock;
};

/* forward declaration */
static void vtm_nm_stream_srv_init_cbs(struct vtm_socket_stream_srv_cbs *cbs);
static void vtm_nm_stream_srv_server_ready(vtm_socket_stream_srv *sock_srv, struct vtm_socket_stream_srv_opts *opts);
static void vtm_nm_stream_srv_worker_init(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd);
static void vtm_nm_stream_srv_worker_end(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd);
static void vtm_nm_stream_srv_sock_connected(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *client);
static void vtm_nm_stream_srv_sock_disconnected(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *client);
static void vtm_nm_stream_srv_sock_can_read(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *client);
static void vtm_nm_stream_srv_sock_can_write(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *client);
static void vtm_nm_stream_srv_sock_error(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *client);

vtm_nm_stream_srv* vtm_nm_stream_srv_new(void)
{
	vtm_nm_stream_srv *srv;

	srv = malloc(sizeof(*srv));
	if (!srv) {
		vtm_err_oom();
		return NULL;
	}

	memset(srv, 0, sizeof(*srv));
	vtm_spinlock_init(&srv->stop_lock);

	return srv;
}

void vtm_nm_stream_srv_free(vtm_nm_stream_srv *srv)
{
	if (!srv)
		return;

	free(srv);
}

int vtm_nm_stream_srv_run(vtm_nm_stream_srv *srv, struct vtm_nm_stream_srv_opts *opts)
{
	int rc;
	struct vtm_socket_stream_srv_opts sock_opts;

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
	sock_opts.addr = opts->addr;
	sock_opts.tls = opts->tls;
	sock_opts.backlog = 25;
	sock_opts.events = 16;
	sock_opts.threads = opts->threads;
	vtm_nm_stream_srv_init_cbs(&sock_opts.cbs);

	/* run stream server */
	vtm_socket_stream_srv_set_usr_data(srv->sock_srv, srv);
	rc = vtm_socket_stream_srv_run(srv->sock_srv, &sock_opts);

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

int vtm_nm_stream_srv_stop(vtm_nm_stream_srv *srv)
{
	int rc;

	vtm_spinlock_lock(&srv->stop_lock);

	if (srv->sock_srv)
		rc = vtm_socket_stream_srv_stop(srv->sock_srv);
	else
		rc = VTM_E_INVALID_STATE;

	vtm_spinlock_unlock(&srv->stop_lock);

	return rc;
}

static void vtm_nm_stream_srv_init_cbs(struct vtm_socket_stream_srv_cbs *cbs)
{
	cbs->server_ready = vtm_nm_stream_srv_server_ready;
	cbs->worker_init = vtm_nm_stream_srv_worker_init;
	cbs->worker_end = vtm_nm_stream_srv_worker_end;
	cbs->sock_connected = vtm_nm_stream_srv_sock_connected;
	cbs->sock_disconnected = vtm_nm_stream_srv_sock_disconnected;
	cbs->sock_can_read = vtm_nm_stream_srv_sock_can_read;
	cbs->sock_can_write = vtm_nm_stream_srv_sock_can_write;
	cbs->sock_error = vtm_nm_stream_srv_sock_error;
}

static void vtm_nm_stream_srv_server_ready(vtm_socket_stream_srv *sock_srv, struct vtm_socket_stream_srv_opts *opts)
{
	vtm_nm_stream_srv *srv;

	srv = vtm_socket_stream_srv_get_usr_data(sock_srv);
	VTM_ASSERT(srv);

	vtm_spinlock_unlock(&srv->stop_lock);

	if (srv->cbs.server_ready)
		srv->cbs.server_ready(srv, srv->opts);
}

static void vtm_nm_stream_srv_worker_init(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd)
{
	vtm_nm_stream_srv *srv;

	srv = vtm_socket_stream_srv_get_usr_data(sock_srv);
	VTM_ASSERT(srv);

	if (srv->cbs.worker_init)
		srv->cbs.worker_init(srv, wd);
}

static void vtm_nm_stream_srv_worker_end(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd)
{
	vtm_nm_stream_srv *srv;

	srv = vtm_socket_stream_srv_get_usr_data(sock_srv);
	VTM_ASSERT(srv);

	if (srv->cbs.worker_end)
		srv->cbs.worker_end(srv, wd);
}

static void vtm_nm_stream_srv_sock_connected(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *client)
{
	vtm_nm_stream_srv *srv;
	vtm_nm_stream_con *con;

	srv = vtm_socket_stream_srv_get_usr_data(sock_srv);
	VTM_ASSERT(srv);

	con = vtm_nm_stream_con_new(client);
	if (!con) {
		vtm_socket_close(client);
		return;
	}

	vtm_socket_set_state(client, VTM_SOCK_STAT_NBL_AUTO | VTM_SOCK_STAT_NBL_READ);
	vtm_socket_set_usr_data(client, con);

	if (srv->cbs.client_connect)
		srv->cbs.client_connect(srv, wd, con);
}

static void vtm_nm_stream_srv_sock_disconnected(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *client)
{
	vtm_nm_stream_srv *srv;
	vtm_nm_stream_con *con;

	srv = vtm_socket_stream_srv_get_usr_data(sock_srv);
	VTM_ASSERT(srv);

	con = vtm_socket_get_usr_data(client);
	if (srv->cbs.client_disconnect && con)
		srv->cbs.client_disconnect(srv, wd, con);

	vtm_socket_set_usr_data(client, NULL);
	vtm_nm_stream_con_free(con);
}

static void vtm_nm_stream_srv_sock_can_read(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *client)
{
	vtm_nm_stream_srv *srv;
	vtm_nm_stream_con *con;
	vtm_dataset *msg;
	enum vtm_net_recv_stat stat;

	srv = vtm_socket_stream_srv_get_usr_data(sock_srv);
	VTM_ASSERT(srv);

	con = vtm_socket_get_usr_data(client);
	VTM_ASSERT(con);

	stat = vtm_nm_stream_con_read(con);
	switch (stat) {
		case VTM_NET_RECV_STAT_ERROR:
		case VTM_NET_RECV_STAT_INVALID:
			vtm_socket_close(client);
			break;

		case VTM_NET_RECV_STAT_COMPLETE:
			msg = vtm_nm_stream_con_get_msg(con);
			if (!msg) {
				vtm_socket_close(client);
				return;
			}

			if (srv->cbs.client_msg)
				srv->cbs.client_msg(srv, wd, con, msg);

			vtm_dataset_free(msg);
			break;

		default:
			break;
	}
}

static void vtm_nm_stream_srv_sock_can_write(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *client)
{
	int rc;
	vtm_nm_stream_con *con;

	con = vtm_socket_get_usr_data(client);
	VTM_ASSERT(con);

	rc = vtm_nm_stream_con_write(con);
	switch (rc) {
		case VTM_OK:
		case VTM_E_IO_AGAIN:
			break;

		default:
			vtm_socket_close(client);
			break;
	}
}

static void vtm_nm_stream_srv_sock_error(vtm_socket_stream_srv *sock_srv, vtm_dataset *wd, vtm_socket *client)
{
	vtm_socket_close(client);
}
