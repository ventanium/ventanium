/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "socket_dgram_server.h"

#include <string.h> /* memset() */

#include <vtm/core/error.h>
#include <vtm/core/squeue.h>
#include <vtm/net/common.h>
#include <vtm/net/socket_listener.h>
#include <vtm/util/atomic.h>
#include <vtm/util/mutex.h>
#include <vtm/util/spinlock.h>
#include <vtm/util/thread.h>

struct vtm_socket_dgram_srv_entry
{
	struct vtm_socket_dgram             dgram;
	struct vtm_socket_dgram_srv_entry   *next;
};

struct vtm_socket_dgram_srv
{
	vtm_socket *socket;
	vtm_socket_listener *listener;
	struct vtm_socket_dgram_srv_cbs cbs;

	void *usr_data;
	vtm_atomic_flag running;
	struct vtm_spinlock stop_lock;

	vtm_thread **threads;
	unsigned int thread_count;

	VTM_SQUEUE_STRUCT(struct vtm_socket_dgram_srv_entry) dgrams;
	vtm_mutex *dgrams_mtx;
	vtm_cond *dgrams_cond_not_full;
	vtm_cond *dgrams_cond_not_empty;
	volatile unsigned int dgrams_count;
	unsigned int dgrams_limit;
};

/* forward declaration */
static int   vtm_socket_dgram_srv_main_run(vtm_socket_dgram_srv *srv);
static void  vtm_socket_dgram_srv_enqueue_dgram(vtm_socket_dgram_srv *srv, struct vtm_socket_dgram_srv_entry *entry);
static void  vtm_socket_dgram_srv_process_dgram(vtm_socket_dgram_srv *srv, vtm_dataset *wd, struct vtm_socket_dgram_srv_entry *entry);
static int   vtm_socket_dgram_srv_workers_span(vtm_socket_dgram_srv *srv, unsigned int threads);
static void  vtm_socket_dgram_srv_workers_interrupt(vtm_socket_dgram_srv *srv);
static void  vtm_socket_dgram_srv_workers_join(vtm_socket_dgram_srv *srv);
static void  vtm_socket_dgram_srv_workers_free(vtm_socket_dgram_srv *srv);
static int   vtm_socket_dgram_srv_worker_run(void *arg);

vtm_socket_dgram_srv* vtm_socket_dgram_srv_new(void)
{
	vtm_socket_dgram_srv *srv;

	srv = malloc(sizeof(vtm_socket_dgram_srv));
	if (!srv) {
		vtm_err_oom();
		return NULL;
	}

	memset(srv, 0, sizeof(vtm_socket_dgram_srv));
	vtm_spinlock_init(&srv->stop_lock);

	return srv;
}

void vtm_socket_dgram_srv_free(vtm_socket_dgram_srv *srv)
{
	free(srv);
}

void* vtm_socket_dgram_srv_get_usr_data(vtm_socket_dgram_srv *srv)
{
	return srv->usr_data;
}

void vtm_socket_dgram_srv_set_usr_data(vtm_socket_dgram_srv *srv, void *data)
{
	srv->usr_data = data;
}

int vtm_socket_dgram_srv_run(vtm_socket_dgram_srv *srv, struct vtm_socket_dgram_srv_opts *opts)
{
	int rc;

	/* lock for init process */
	vtm_spinlock_lock(&srv->stop_lock);

	/* set callbacks */
	srv->cbs = opts->cbs;

	/* create socket */
	srv->socket = vtm_socket_new(opts->addr.family, VTM_SOCK_TYPE_DGRAM);
	if (!srv->socket) {
		rc = vtm_err_get_code();
		goto unlock;
	}

	/* bind socket */
	rc = vtm_socket_bind(srv->socket, opts->addr.host, opts->addr.port);
	if (rc != VTM_OK)
		goto clean_socket;

	/* set non-blocking */
	rc = vtm_socket_set_opt(srv->socket, VTM_SOCK_OPT_NONBLOCKING,
		(bool[]) {true}, sizeof(bool));
	if (rc != VTM_OK)
		goto clean_socket;

	/* create socket listener */
	srv->listener = vtm_socket_listener_new(1);
	if (!srv->listener) {
		rc = vtm_err_get_code();
		goto clean_socket;
	}

	/* add server socket to listener */
	vtm_socket_set_state(srv->socket, VTM_SOCK_STAT_NBL_READ);
	rc = vtm_socket_listener_add(srv->listener, srv->socket);
	if (rc != VTM_OK)
		goto clean_listener;

	/* create worker queue */
	if (opts->threads > 0) {
		VTM_SQUEUE_INIT(srv->dgrams);
		srv->dgrams_mtx = vtm_mutex_new();
		if (!srv->dgrams_mtx) {
			rc = vtm_err_get_code();
			goto clean;
		}

		srv->dgrams_cond_not_empty = vtm_cond_new();
		srv->dgrams_cond_not_full = vtm_cond_new();
		if (!srv->dgrams_cond_not_empty || !srv->dgrams_cond_not_full) {
			rc = vtm_err_get_code();
			goto clean;
		}

		srv->dgrams_limit = opts->queue_limit;
		if (srv->dgrams_limit == 0)
			srv->dgrams_limit = opts->threads * 2;
	}

	/* spawn workers */
	vtm_atomic_flag_set(srv->running);
	rc = vtm_socket_dgram_srv_workers_span(srv, opts->threads);
	if (rc != VTM_OK)
		goto clean;

	/* notify server ready */
	if (srv->cbs.server_ready)
		srv->cbs.server_ready(srv, opts);

	/* init process finished */
	vtm_spinlock_unlock(&srv->stop_lock);

	/* run socket listener */
	vtm_socket_dgram_srv_main_run(srv);

	/* end for workers */
	vtm_socket_dgram_srv_workers_interrupt(srv);
	vtm_socket_dgram_srv_workers_join(srv);
	vtm_socket_dgram_srv_workers_free(srv);

	/* cleanup */
	vtm_spinlock_lock(&srv->stop_lock);

clean:
	if (opts->threads > 0)
		VTM_SQUEUE_CLEAR(srv->dgrams, struct vtm_socket_dgram_srv_entry, free);

	vtm_cond_free(srv->dgrams_cond_not_empty);
	vtm_cond_free(srv->dgrams_cond_not_full);
	vtm_mutex_free(srv->dgrams_mtx);

clean_listener:
	vtm_socket_listener_remove(srv->listener, srv->socket);
	vtm_socket_listener_free(srv->listener);
	srv->listener = NULL;

clean_socket:
	vtm_socket_close(srv->socket);
	vtm_socket_free(srv->socket);

unlock:
	vtm_spinlock_unlock(&srv->stop_lock);

	return rc;
}

int vtm_socket_dgram_srv_stop(vtm_socket_dgram_srv *srv)
{
	int rc;

	vtm_spinlock_lock(&srv->stop_lock);

	vtm_atomic_flag_unset(srv->running);
	if (srv->listener)
		rc = vtm_socket_listener_interrupt(srv->listener);
	else
		rc = VTM_E_INVALID_STATE;

	vtm_spinlock_unlock(&srv->stop_lock);

	return rc;
}

int vtm_socket_dgram_srv_send(vtm_socket_dgram_srv *srv, void *buf, size_t len, const struct vtm_socket_saddr *saddr)
{
	int rc;
	size_t bytes_sent;

	rc = vtm_socket_dgram_send(srv->socket, buf, len, &bytes_sent, saddr);
	if (rc != VTM_OK)
		return rc;

	if (bytes_sent != len)
		return vtm_err_set(VTM_E_IO_UNKNOWN);

	return VTM_OK;
}

static int vtm_socket_dgram_srv_main_run(vtm_socket_dgram_srv *srv)
{
	int rc;
	vtm_dataset *wd;
	struct vtm_socket_dgram_srv_entry *entry;
	struct vtm_socket_event *events;
	size_t num_events;

	/* GCC warns about uninitialized usage if optimizations turned on */
	wd = NULL;

	entry = malloc(sizeof(*entry));
	if (!entry) {
		vtm_err_oom();
		return vtm_err_get_code();
	}

	if (srv->thread_count == 0) {
		wd = vtm_dataset_new();
		if (!wd)
			return vtm_err_get_code();

		if (srv->cbs.worker_init)
			srv->cbs.worker_init(srv, wd);
	}

	while (vtm_atomic_flag_isset(srv->running)) {
		rc = vtm_socket_listener_run(srv->listener, &events, &num_events);
		if (rc != VTM_OK)
			goto finish;

		if (num_events == 0)
			continue;

		if (events[0].sock != srv->socket ||
			(events[0].events & VTM_SOCK_EVT_READ) == 0)
			goto finish;

		vtm_buf_init(&entry->dgram.buf, VTM_NET_BYTEORDER);
		rc = vtm_socket_dgram_recv(srv->socket, entry->dgram.buf.data,
			entry->dgram.buf.len, &entry->dgram.buf.used, &entry->dgram.saddr);
		if (rc != VTM_OK)
			goto finish;

		if (srv->thread_count == 0) {
			vtm_socket_dgram_srv_process_dgram(srv, wd, entry);
		}
		else {
			vtm_socket_dgram_srv_enqueue_dgram(srv, entry);
			entry = malloc(sizeof(*entry));
			if (!entry) {
				vtm_err_oom();
				rc = vtm_err_get_code();
				goto finish;
			}
		}

		vtm_socket_listener_rearm(srv->listener, srv->socket);
	}

finish:
	free(entry);

	if (srv->thread_count == 0) {
		if (srv->cbs.worker_end)
			srv->cbs.worker_end(srv, wd);

		vtm_dataset_free(wd);
	}

	vtm_atomic_flag_unset(srv->running);

	return rc;
}

static void vtm_socket_dgram_srv_enqueue_dgram(vtm_socket_dgram_srv *srv, struct vtm_socket_dgram_srv_entry *entry)
{
	vtm_mutex_lock(srv->dgrams_mtx);
	while (srv->dgrams_count >= srv->dgrams_limit) {
		if (!vtm_atomic_flag_isset(srv->running))
			goto unlock;
		vtm_cond_wait(srv->dgrams_cond_not_full, srv->dgrams_mtx);
	}
	VTM_SQUEUE_ADD(srv->dgrams, entry);
	srv->dgrams_count++;
	vtm_cond_signal_all(srv->dgrams_cond_not_empty);
unlock:
	vtm_mutex_unlock(srv->dgrams_mtx);
}

static int vtm_socket_dgram_srv_workers_span(vtm_socket_dgram_srv *srv, unsigned int threads)
{
	unsigned int i;

	srv->thread_count = threads;
	if (srv->thread_count == 0)
		return VTM_OK;

	srv->threads = calloc(srv->thread_count, sizeof(vtm_thread*));
	if (!srv->threads) {
		vtm_err_oom();
		return vtm_err_get_code();
	}

	for (i=0; i < srv->thread_count; i++) {
		srv->threads[i] = vtm_thread_new(vtm_socket_dgram_srv_worker_run, srv);
		if (!srv->threads[i])
			return VTM_ERROR;
	}

	return VTM_OK;
}

static void vtm_socket_dgram_srv_workers_interrupt(vtm_socket_dgram_srv *srv)
{
	if (!srv->dgrams_cond_not_empty)
		return;

	vtm_cond_signal_all(srv->dgrams_cond_not_empty);
}

static void vtm_socket_dgram_srv_workers_join(vtm_socket_dgram_srv *srv)
{
	vtm_thread *th;
	unsigned int i;

	if (srv->thread_count == 0)
		return;

	for (i=0; i < srv->thread_count; i++) {
		th = srv->threads[i];
		vtm_thread_join(th);
	}
}

static void vtm_socket_dgram_srv_workers_free(vtm_socket_dgram_srv *srv)
{
	vtm_thread *th;
	unsigned int i;

	if (srv->thread_count == 0)
		return;

	for (i=0; i < srv->thread_count; i++) {
		th = srv->threads[i];
		vtm_thread_free(th);
	}

	free(srv->threads);
}

static int vtm_socket_dgram_srv_worker_run(void *arg)
{
	vtm_dataset *wd;
	struct vtm_socket_dgram_srv_entry *entry;
	vtm_socket_dgram_srv *srv;

	srv = arg;
	wd = vtm_dataset_new();

	if (srv->cbs.worker_init)
		srv->cbs.worker_init(srv, wd);

	while (vtm_atomic_flag_isset(srv->running)) {
		vtm_mutex_lock(srv->dgrams_mtx);

		while (VTM_SQUEUE_IS_EMPTY(srv->dgrams)) {
			if (!vtm_atomic_flag_isset(srv->running)) {
				vtm_mutex_unlock(srv->dgrams_mtx);
				goto finish;
			}
			vtm_cond_wait(srv->dgrams_cond_not_empty, srv->dgrams_mtx);
		}

		VTM_SQUEUE_POLL(srv->dgrams, entry);
		srv->dgrams_count--;
		vtm_cond_signal_all(srv->dgrams_cond_not_full);
		vtm_mutex_unlock(srv->dgrams_mtx);

		vtm_socket_dgram_srv_process_dgram(srv, wd, entry);

		free(entry);
	}

finish:

	if (srv->cbs.worker_end)
		srv->cbs.worker_end(srv, wd);

	vtm_dataset_free(wd);

	return VTM_OK;
}

static void vtm_socket_dgram_srv_process_dgram(vtm_socket_dgram_srv *srv, vtm_dataset *wd, struct vtm_socket_dgram_srv_entry *entry)
{
	if (srv->cbs.dgram_recv)
		srv->cbs.dgram_recv(srv, wd, &entry->dgram);
}
