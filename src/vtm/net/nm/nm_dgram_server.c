/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "nm_dgram_server.h"

#include <string.h> /* memset() */
#include <vtm/core/error.h>
#include <vtm/net/socket_dgram_server.h>
#include <vtm/net/nm/nm_parser_intl.h>
#include <vtm/net/nm/nm_protocol_intl.h>
#include <vtm/util/spinlock.h>

struct vtm_nm_dgram_srv
{
	vtm_socket_dgram_srv           *sock_srv;
	struct vtm_nm_dgram_srv_opts   *opts;
	struct vtm_nm_dgram_srv_cbs    cbs;
	struct vtm_spinlock            stop_lock;
};

/* forward declaration */
static void vtm_nm_dgram_srv_init_cbs(struct vtm_socket_dgram_srv_cbs *cbs);
static void vtm_nm_dgram_srv_server_ready(vtm_socket_dgram_srv *sock_srv, struct vtm_socket_dgram_srv_opts *opts);
static void vtm_nm_dgram_srv_worker_init(vtm_socket_dgram_srv *sock_srv, vtm_dataset *wd);
static void vtm_nm_dgram_srv_worker_end(vtm_socket_dgram_srv *sock_srv, vtm_dataset *wd);
static void vtm_nm_dgram_srv_dgram_recv(vtm_socket_dgram_srv *sock_srv, vtm_dataset *wd, struct vtm_socket_dgram *dgram);

vtm_nm_dgram_srv* vtm_nm_dgram_srv_new(void)
{
	vtm_nm_dgram_srv *srv;

	srv = malloc(sizeof(*srv));
	if (!srv) {
		vtm_err_oom();
		return NULL;
	}

	memset(srv, 0, sizeof(*srv));
	vtm_spinlock_init(&srv->stop_lock);

	return srv;
}

void vtm_nm_dgram_srv_free(vtm_nm_dgram_srv *srv)
{
	if (!srv)
		return;

	free(srv);
}

int vtm_nm_dgram_srv_run(vtm_nm_dgram_srv *srv, struct vtm_nm_dgram_srv_opts *opts)
{
	int rc;
	struct vtm_socket_dgram_srv_opts sock_opts;

	/* create dgram server */
	vtm_spinlock_lock(&srv->stop_lock);
	srv->sock_srv = vtm_socket_dgram_srv_new();
	if (!srv->sock_srv) {
		rc = vtm_err_get_code();
		goto unlock;
	}

	/* set callbacks */
	srv->opts = opts;
	srv->cbs = opts->cbs;

	/* prepare dgram server options */
	sock_opts.addr = opts->addr;
	sock_opts.queue_limit = 0;
	sock_opts.threads = opts->threads;
	vtm_nm_dgram_srv_init_cbs(&sock_opts.cbs);

	/* run dgram server */
	vtm_socket_dgram_srv_set_usr_data(srv->sock_srv, srv);
	rc = vtm_socket_dgram_srv_run(srv->sock_srv, &sock_opts);

	/* if run was successful, stop_lock was unlocked in server_ready callback */
	if (rc == VTM_OK)
		vtm_spinlock_lock(&srv->stop_lock);

	/* free dgram server */
	vtm_socket_dgram_srv_free(srv->sock_srv);
	srv->sock_srv = NULL;

unlock:
	vtm_spinlock_unlock(&srv->stop_lock);

	return rc;
}

int vtm_nm_dgram_srv_stop(vtm_nm_dgram_srv *srv)
{
	int rc;

	vtm_spinlock_lock(&srv->stop_lock);

	if (srv->sock_srv)
		rc = vtm_socket_dgram_srv_stop(srv->sock_srv);
	else
		rc = VTM_E_INVALID_STATE;

	vtm_spinlock_unlock(&srv->stop_lock);

	return rc;
}

int vtm_nm_dgram_srv_send(vtm_nm_dgram_srv *srv, vtm_dataset *msg, const struct vtm_socket_saddr *saddr)
{
	int rc;
	struct vtm_buf buf;

	vtm_buf_init(&buf, VTM_NET_BYTEORDER);

	rc = vtm_nm_msg_to_buf(msg, &buf);
	if (rc != VTM_OK)
		goto end;

	rc = vtm_socket_dgram_srv_send(srv->sock_srv, buf.data, buf.used, saddr);

end:
	vtm_buf_release(&buf);

	return rc;
}

static void vtm_nm_dgram_srv_init_cbs(struct vtm_socket_dgram_srv_cbs *cbs)
{
	cbs->server_ready = vtm_nm_dgram_srv_server_ready;
	cbs->worker_init  = vtm_nm_dgram_srv_worker_init;
	cbs->worker_end   = vtm_nm_dgram_srv_worker_end;
	cbs->dgram_recv   = vtm_nm_dgram_srv_dgram_recv;
}

static void vtm_nm_dgram_srv_server_ready(vtm_socket_dgram_srv *sock_srv, struct vtm_socket_dgram_srv_opts *opts)
{
	vtm_nm_dgram_srv *srv;

	srv = vtm_socket_dgram_srv_get_usr_data(sock_srv);
	VTM_ASSERT(srv);

	vtm_spinlock_unlock(&srv->stop_lock);

	if (srv->cbs.server_ready)
		srv->cbs.server_ready(srv, srv->opts);
}

static void vtm_nm_dgram_srv_worker_init(vtm_socket_dgram_srv *sock_srv, vtm_dataset *wd)
{
	vtm_nm_dgram_srv *srv;

	srv = vtm_socket_dgram_srv_get_usr_data(sock_srv);
	if (!srv)
		return;

	if (srv->cbs.worker_init)
		srv->cbs.worker_init(srv, wd);
}

static void vtm_nm_dgram_srv_worker_end(vtm_socket_dgram_srv *sock_srv, vtm_dataset *wd)
{
	vtm_nm_dgram_srv *srv;

	srv = vtm_socket_dgram_srv_get_usr_data(sock_srv);
	if (!srv)
		return;

	if (srv->cbs.worker_end)
		srv->cbs.worker_end(srv, wd);
}

static void vtm_nm_dgram_srv_dgram_recv(vtm_socket_dgram_srv *sock_srv, vtm_dataset *wd, struct vtm_socket_dgram *dgram)
{
	int rc;
	vtm_nm_dgram_srv *srv;
	vtm_dataset *msg;

	srv = vtm_socket_dgram_srv_get_usr_data(sock_srv);
	if (!srv)
		return;

	if (!srv->cbs.msg_recv)
		return;

	msg = vtm_dataset_new();
	if (!msg)
		return;

	rc = vtm_nm_msg_from_buf(msg, &dgram->buf);
	if (rc == VTM_OK)
		srv->cbs.msg_recv(srv, wd, msg, &dgram->saddr);

	vtm_dataset_free(msg);
}
