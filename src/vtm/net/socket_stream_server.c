/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "socket_stream_server.h"

#include <string.h> /* memset() */

#include <vtm/core/error.h>
#include <vtm/core/lang.h>
#include <vtm/core/list.h>
#include <vtm/core/map.h>
#include <vtm/core/squeue.h>
#include <vtm/net/socket_intl.h>
#include <vtm/net/socket_listener.h>
#include <vtm/util/atomic.h>
#include <vtm/util/latch.h>
#include <vtm/util/mutex.h>
#include <vtm/util/spinlock.h>
#include <vtm/util/thread.h>

enum vtm_socket_stream_srv_entry_type
{
	VTM_SOCK_SRV_ACCEPTED,
	VTM_SOCK_SRV_READ,
	VTM_SOCK_SRV_WRITE,
	VTM_SOCK_SRV_CLOSED,
	VTM_SOCK_SRV_ERROR
};

struct vtm_socket_stream_srv_entry
{
	vtm_socket *sock;
	enum vtm_socket_stream_srv_entry_type type;

	struct vtm_socket_stream_srv_entry *next;
};

struct vtm_socket_stream_srv
{
	vtm_socket *socket;
	vtm_socket_listener *listener;
	struct vtm_socket_stream_srv_cbs cbs;

	void *usr_data;
	vtm_atomic_flag running;
	struct vtm_spinlock stop_lock;

	vtm_thread **threads;
	unsigned int thread_count;
	struct vtm_latch drain_prepare_latch;
	struct vtm_latch drain_run_latch;

	VTM_SQUEUE_STRUCT(struct vtm_socket_stream_srv_entry) events;
	vtm_mutex *events_mtx;
	vtm_cond *events_cond;
	vtm_list *release_socks;
	vtm_list *relay_events;

	vtm_map *cons;
	vtm_mutex *cons_mtx;
};

/* forward declaration */
static int  vtm_socket_stream_srv_create_socket(vtm_socket_stream_srv *srv, struct vtm_socket_stream_srv_opts *opts);
static int  vtm_socket_stream_srv_prepare_socket(vtm_socket_stream_srv *srv, struct vtm_socket_stream_srv_opts *opts);
static int  vtm_socket_stream_srv_main_run(vtm_socket_stream_srv *srv);
static int  vtm_socket_stream_srv_handle_direct(vtm_socket_stream_srv *srv, struct vtm_socket_event *events, size_t num_events, vtm_dataset *wd);
static int  vtm_socket_stream_srv_handle_queued(vtm_socket_stream_srv *srv, struct vtm_socket_event *events, size_t num_events);
static void vtm_socket_stream_srv_drain_direct(vtm_socket_stream_srv *srv, vtm_dataset *wd);
static void vtm_socket_stream_srv_drain_queued(vtm_socket_stream_srv *srv, vtm_dataset *wd);
static int  vtm_socket_stream_srv_accept(vtm_socket_stream_srv *srv, vtm_dataset *wd, bool direct);
static int  vtm_socket_stream_srv_create_event(vtm_socket_stream_srv *srv, enum vtm_socket_stream_srv_entry_type type, vtm_socket *sock);
static void vtm_socket_stream_srv_add_event(vtm_socket_stream_srv *srv, struct vtm_socket_stream_srv_entry *event);
static int  vtm_socket_stream_srv_create_relay_event(vtm_socket_stream_srv *srv, enum vtm_socket_stream_srv_entry_type type, vtm_socket *sock);
static int  vtm_socket_stream_srv_workers_span(vtm_socket_stream_srv *srv, unsigned int threads);
static void vtm_socket_stream_srv_workers_interrupt(vtm_socket_stream_srv *srv);
static void vtm_socket_stream_srv_workers_join(vtm_socket_stream_srv *srv);
static void vtm_socket_stream_srv_workers_free(vtm_socket_stream_srv *srv);
static int  vtm_socket_stream_srv_worker_run(void *arg);
static void vtm_socket_stream_srv_lock_cons(vtm_socket_stream_srv *srv);
static void vtm_socket_stream_srv_unlock_cons(vtm_socket_stream_srv *srv);
static void vtm_socket_stream_srv_free_sockets(vtm_socket_stream_srv *srv);

/* socket functions */
static bool vtm_socket_stream_srv_sock_event(vtm_socket_stream_srv *srv, vtm_dataset *wd, struct vtm_socket_stream_srv_entry *event);
static int  vtm_socket_stream_srv_sock_check(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *sock, bool rearm);
static void vtm_socket_stream_srv_sock_accepted(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *sock);
static void vtm_socket_stream_srv_sock_can_read(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *sock);
static void vtm_socket_stream_srv_sock_can_write(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *sock);
static void vtm_socket_stream_srv_sock_closed(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *sock);
static void vtm_socket_stream_srv_sock_error(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *sock);
static void vtm_socket_stream_srv_sock_free(vtm_socket_stream_srv *srv, vtm_socket *sock);
static void vtm_socket_stream_srv_sock_init_cbs(vtm_socket_stream_srv *srv, vtm_socket *sock);
static int  vtm_socket_stream_srv_sock_cb_update(void *stream_srv, vtm_socket *sock);
static int  vtm_socket_stream_srv_sock_trylock(vtm_socket *sock, unsigned int flags);
static void vtm_socket_stream_srv_sock_unlock(vtm_socket *sock, unsigned int flags);

vtm_socket_stream_srv* vtm_socket_stream_srv_new(void)
{
	vtm_socket_stream_srv *srv;

	srv = malloc(sizeof(vtm_socket_stream_srv));
	if (!srv) {
		vtm_err_oom();
		return NULL;
	}

	memset(srv, 0, sizeof(*srv));
	vtm_spinlock_init(&srv->stop_lock);

	return srv;
}

void vtm_socket_stream_srv_free(vtm_socket_stream_srv *srv)
{
	free(srv);
}

void* vtm_socket_stream_srv_get_usr_data(vtm_socket_stream_srv *srv)
{
	return srv->usr_data;
}

void vtm_socket_stream_srv_set_usr_data(vtm_socket_stream_srv *srv, void *data)
{
	srv->usr_data = data;
}

int vtm_socket_stream_srv_run(vtm_socket_stream_srv *srv, struct vtm_socket_stream_srv_opts *opts)
{
	int rc;

	/* lock for init process */
	vtm_spinlock_lock(&srv->stop_lock);

	/* set callbacks */
	srv->cbs = opts->cbs;

	/* create socket */
	rc = vtm_socket_stream_srv_create_socket(srv, opts);
	if (rc != VTM_OK)
		goto unlock;

	/* prepare socket */
	rc = vtm_socket_stream_srv_prepare_socket(srv, opts);
	if (rc != VTM_OK)
		goto clean_socket;

	/* create socket listener */
	srv->listener = vtm_socket_listener_new(opts->events);
	if (!srv->listener) {
		rc = vtm_err_get_code();
		goto clean_socket;
	}

	/* add server socket to listener */
	vtm_socket_set_state(srv->socket, VTM_SOCK_STAT_NBL_READ);
	rc = vtm_socket_listener_add(srv->listener, srv->socket);
	if (rc != VTM_OK)
		goto clean_listener;

	/* create map for connections */
	srv->cons = vtm_map_new(VTM_ELEM_POINTER, VTM_ELEM_POINTER, 64);
	if (!srv->cons) {
		rc = vtm_err_get_code();
		goto clean_listener;
	}

	/* relay event list */
	srv->relay_events = vtm_list_new(VTM_ELEM_POINTER, 8);
	if (!srv->relay_events) {
		rc = vtm_err_get_code();
		goto clean;
	}
	vtm_list_set_free_func(srv->relay_events, free);

	if (opts->threads > 0) {
		/* create synch helpers */
		vtm_latch_init(&srv->drain_prepare_latch, opts->threads);
		vtm_latch_init(&srv->drain_run_latch, 1);
		srv->cons_mtx = vtm_mutex_new();
		if (!srv->cons_mtx)
			goto clean;

		/* create worker queue */
		VTM_SQUEUE_INIT(srv->events);
		srv->events_mtx = vtm_mutex_new();
		if (!srv->events_mtx) {
			rc = vtm_err_get_code();
			goto clean;
		}

		srv->events_cond = vtm_cond_new();
		if (!srv->events_cond) {
			rc = vtm_err_get_code();
			goto clean;
		}

		/* releaseable sockets */
		srv->release_socks = vtm_list_new(VTM_ELEM_POINTER, 8);
		if (!srv->release_socks) {
			rc = vtm_err_get_code();
			goto clean;
		}
	}

	/* spawn workers */
	vtm_atomic_flag_set(srv->running);
	rc = vtm_socket_stream_srv_workers_span(srv, opts->threads);
	if (rc != VTM_OK)
		goto clean;

	/* notify server ready */
	if (srv->cbs.server_ready)
		srv->cbs.server_ready(srv, opts);

	/* init process finished */
	vtm_spinlock_unlock(&srv->stop_lock);

	/* run queue listener */
	vtm_socket_stream_srv_main_run(srv);

	/* end for workers */
	vtm_socket_stream_srv_workers_interrupt(srv);
	vtm_socket_stream_srv_workers_join(srv);
	vtm_socket_stream_srv_workers_free(srv);

	/* cleanup */
	vtm_spinlock_lock(&srv->stop_lock);

clean:
	if (opts->threads > 0) {
		vtm_socket_stream_srv_free_sockets(srv);
		VTM_SQUEUE_CLEAR(srv->events, struct vtm_socket_stream_srv_entry, free);
		vtm_cond_free(srv->events_cond);
		vtm_mutex_free(srv->events_mtx);
		vtm_mutex_free(srv->cons_mtx);
		vtm_list_free(srv->release_socks);
		vtm_latch_release(&srv->drain_prepare_latch);
		vtm_latch_release(&srv->drain_run_latch);
	}

	vtm_list_free(srv->relay_events);
	vtm_map_free(srv->cons);

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

int vtm_socket_stream_srv_stop(vtm_socket_stream_srv *srv)
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

static int vtm_socket_stream_srv_create_socket(vtm_socket_stream_srv *srv, struct vtm_socket_stream_srv_opts *opts)
{
	struct vtm_socket_tls_opts tls_opts;

	if (opts->tls.enabled) {
		tls_opts.is_server = true;
		tls_opts.cert_file = opts->tls.cert_file;
		tls_opts.key_file = opts->tls.key_file;
		tls_opts.ciphers = opts->tls.ciphers;
		srv->socket = vtm_socket_tls_new(opts->addr.family, &tls_opts);
	}
	else {
		srv->socket = vtm_socket_new(opts->addr.family, VTM_SOCK_TYPE_STREAM);
	}

	if (!srv->socket)
		return VTM_ERROR;

	return VTM_OK;
}

static int vtm_socket_stream_srv_prepare_socket(vtm_socket_stream_srv *srv, struct vtm_socket_stream_srv_opts *opts)
{
	int rc;

	/* set non-blocking */
	rc = vtm_socket_set_opt(srv->socket, VTM_SOCK_OPT_NONBLOCKING,
		(bool[]) {true}, sizeof(bool));
	if (rc != VTM_OK)
		return rc;

	/* bind socket */
	rc = vtm_socket_bind(srv->socket, opts->addr.host, opts->addr.port);
	if (rc != VTM_OK)
		return rc;

	/* listen socket*/
	return vtm_socket_listen(srv->socket, opts->backlog);
}

static int vtm_socket_stream_srv_main_run(vtm_socket_stream_srv *srv)
{
	int rc;
	struct vtm_socket_event *events;
	size_t num_events;
	vtm_dataset *wd;

	/* GCC warns about uninitialized usage if optimizations turned on */
	wd = NULL;

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

		if (srv->thread_count == 0)
			rc = vtm_socket_stream_srv_handle_direct(srv, events, num_events, wd);
		else
			rc = vtm_socket_stream_srv_handle_queued(srv, events, num_events);

		if (rc != VTM_OK)
			goto finish;
	}

finish:
	vtm_atomic_flag_unset(srv->running);

	if (srv->thread_count == 0) {
		vtm_socket_stream_srv_drain_direct(srv, wd);

		if (srv->cbs.worker_end)
			srv->cbs.worker_end(srv, wd);

		vtm_dataset_free(wd);
	}
	else {
		vtm_socket_stream_srv_drain_queued(srv, wd);
	}

	return rc;
}

static int vtm_socket_stream_srv_handle_direct(vtm_socket_stream_srv *srv, struct vtm_socket_event *events, size_t num_events, vtm_dataset *wd)
{
	int rc;
	size_t i;
	vtm_socket *sock;
	struct vtm_socket_stream_srv_entry *event;

	/* handle relay events */
	while (vtm_list_size(srv->relay_events) > 0) {
		event = vtm_list_get_pointer(srv->relay_events, 0);
		switch (event->type) {
			case VTM_SOCK_SRV_CLOSED:
				vtm_socket_set_state(event->sock, VTM_SOCK_STAT_CLOSED);
				vtm_socket_stream_srv_sock_closed(srv, wd, event->sock);
				break;

			case VTM_SOCK_SRV_ERROR:
				vtm_socket_stream_srv_sock_error(srv, wd, event->sock);
				break;

			default:
				break;
		}
		vtm_list_remove(srv->relay_events, 0);
		free(event);
	}

	/* handle events from listener */
	for (i=0; i < num_events; i++) {
		sock = events[i].sock;
		if (events[i].events & VTM_SOCK_EVT_CLOSED) {
			vtm_socket_set_state(sock, VTM_SOCK_STAT_CLOSED);
			vtm_socket_stream_srv_sock_closed(srv, wd, sock);
		}
		else if (events[i].events & VTM_SOCK_EVT_ERROR) {
			vtm_socket_stream_srv_sock_error(srv, wd, sock);
		}
		else {
			if (events[i].events & VTM_SOCK_EVT_READ) {
				if (sock == srv->socket) {
					rc = vtm_socket_stream_srv_accept(srv, wd, true);
					if (rc != VTM_OK)
						return rc;
					continue;
				}
				vtm_socket_stream_srv_sock_can_read(srv, wd, sock);
			}
			if (events[i].events & VTM_SOCK_EVT_WRITE) {
				vtm_socket_stream_srv_sock_can_write(srv, wd, sock);
			}
		}
	}

	return VTM_OK;
}

static int vtm_socket_stream_srv_handle_queued(vtm_socket_stream_srv *srv, struct vtm_socket_event *events, size_t num_events)
{
	int rc, errc;
	size_t i, count;
	vtm_socket *sock, *acc;
	struct vtm_socket_stream_srv_entry *event;

	errc = 0;
	acc = NULL;

	vtm_mutex_lock(srv->events_mtx);

	/* check for releaseable sockets */
	count = vtm_list_size(srv->release_socks);
	for (i=0; i < count; i++) {
		sock = vtm_list_get_pointer(srv->release_socks, i);
		vtm_socket_enable_free_on_unref(sock);
	}
	vtm_list_clear(srv->release_socks);

	/* process relay events */
	while (vtm_list_size(srv->relay_events) > 0) {
		event = vtm_list_get_pointer(srv->relay_events, 0);
		vtm_list_remove(srv->relay_events, 0);
		VTM_SQUEUE_ADD(srv->events, event);
	}

	/* process events from listener */
	for (i=0; i < num_events; i++) {
		sock = events[i].sock;

		/* closed */
		if (events[i].events & VTM_SOCK_EVT_CLOSED) {
			vtm_socket_set_state(sock, VTM_SOCK_STAT_CLOSED);
			rc = vtm_socket_stream_srv_create_event(srv, VTM_SOCK_SRV_CLOSED, sock);
			if (rc != VTM_OK) {
				errc++;
				break;
			}
			continue;
		}

		/* error */
		if (events[i].events & VTM_SOCK_EVT_ERROR) {
			rc = vtm_socket_stream_srv_create_event(srv, VTM_SOCK_SRV_ERROR, sock);
			if (rc != VTM_OK) {
				errc++;
				break;
			}
			continue;
		}

		/* read */
		if (events[i].events & VTM_SOCK_EVT_READ) {
			if (sock == srv->socket) {
				acc = sock;
				continue;
			}

			rc = vtm_socket_stream_srv_create_event(srv, VTM_SOCK_SRV_READ, sock);
			if (rc != VTM_OK) {
				errc++;
				break;
			}
		}

		/* write */
		if (events[i].events & VTM_SOCK_EVT_WRITE) {
			rc = vtm_socket_stream_srv_create_event(srv, VTM_SOCK_SRV_WRITE, sock);
			if (rc != VTM_OK) {
				errc++;
				break;
			}
		}
	}

	vtm_cond_signal_all(srv->events_cond);
	vtm_mutex_unlock(srv->events_mtx);

	if (errc > 0)
		return VTM_ERROR;

	if (acc) {
		rc = vtm_socket_stream_srv_accept(srv, NULL, false);
		if (rc != VTM_OK)
			return rc;
	}

	return VTM_OK;
}

static void vtm_socket_stream_srv_drain_direct(vtm_socket_stream_srv *srv, vtm_dataset *wd)
{
	vtm_list *entries;
	struct vtm_map_entry *entry;
	size_t i, count;
	vtm_socket *sock;

	entries = vtm_map_entryset(srv->cons);
	if (!entries)
		return;

	count = vtm_list_size(entries);
	for (i=0; i < count; i++) {
		entry = vtm_list_get_pointer(entries,i);
		sock = entry->key.elem_pointer;

		vtm_socket_close(sock);
		vtm_socket_stream_srv_sock_closed(srv, wd, sock);
	}

	vtm_list_free(entries);
}

static void vtm_socket_stream_srv_drain_queued(vtm_socket_stream_srv *srv, vtm_dataset *wd)
{
	int rc;
	vtm_list *entries;
	struct vtm_socket_stream_srv_entry *event;
	struct vtm_map_entry *entry;
	size_t i, count;
	vtm_socket *sock;

	/* wait for all threads to end event processing */
	vtm_cond_signal_all(srv->events_cond);
	vtm_latch_await(&srv->drain_prepare_latch);

	vtm_socket_stream_srv_lock_cons(srv);
	vtm_mutex_lock(srv->events_mtx);

	/* clear all pending events */
	VTM_SQUEUE_FOR_EACH(srv->events, event)
		vtm_socket_unref(event->sock);
	VTM_SQUEUE_CLEAR(srv->events, struct vtm_socket_stream_srv_entry, free);
	entries = vtm_map_entryset(srv->cons);
	if (!entries)
		goto unlock;

	count = vtm_list_size(entries);
	for (i=0; i < count; i++) {
		entry = vtm_list_get_pointer(entries,i);
		sock = entry->key.elem_pointer;
		vtm_socket_close(sock);

		rc = vtm_socket_stream_srv_create_event(srv, VTM_SOCK_SRV_CLOSED, sock);
		if (rc != VTM_OK)
			goto unlock;
	}
	vtm_list_free(entries);

unlock:
	vtm_mutex_unlock(srv->events_mtx);
	vtm_socket_stream_srv_unlock_cons(srv);

	/* let all waiting threads process close events */
	vtm_latch_count(&srv->drain_run_latch);
}

static int vtm_socket_stream_srv_create_event(vtm_socket_stream_srv *srv, enum vtm_socket_stream_srv_entry_type type, vtm_socket *sock)
{
	struct vtm_socket_stream_srv_entry *event;

	event = malloc(sizeof(*event));
	if (!event) {
		vtm_err_oom();
		return vtm_err_get_code();
	}

	vtm_socket_ref(sock);

	event->sock = sock;
	event->type = type;
	VTM_SQUEUE_ADD(srv->events, event);

	return VTM_OK;
}

static void vtm_socket_stream_srv_add_event(vtm_socket_stream_srv *srv, struct vtm_socket_stream_srv_entry *event)
{
	vtm_socket_ref(event->sock);
	vtm_mutex_lock(srv->events_mtx);
	VTM_SQUEUE_ADD(srv->events, event);
	vtm_cond_signal(srv->events_cond);
	vtm_mutex_unlock(srv->events_mtx);
}

static int vtm_socket_stream_srv_create_relay_event(vtm_socket_stream_srv *srv, enum vtm_socket_stream_srv_entry_type type, vtm_socket *sock)
{
	struct vtm_socket_stream_srv_entry *event;

	event = malloc(sizeof(*event));
	if (!event) {
		vtm_err_oom();
		return vtm_err_get_code();
	}

	event->type = type;
	event->sock = sock;

	if (srv->thread_count > 0)
		vtm_mutex_lock(srv->events_mtx);

	vtm_list_add_va(srv->relay_events, event);

	if (srv->thread_count > 0)
		vtm_mutex_unlock(srv->events_mtx);

	return VTM_OK;
}

static int vtm_socket_stream_srv_accept(vtm_socket_stream_srv *srv, vtm_dataset *wd, bool direct)
{
	int rc;
	vtm_socket *client;
	struct vtm_socket_stream_srv_entry *node;

	while (true) {
		rc = vtm_socket_accept(srv->socket, &client);
		if (rc != VTM_OK) {
			switch (rc) {
				case VTM_E_IO_AGAIN:
					return vtm_socket_listener_rearm(srv->listener, srv->socket);

				case VTM_E_MAX_REACHED:
				case VTM_E_MEMORY:
					return VTM_OK;

				case VTM_E_INTERRUPTED:
				case VTM_E_PERMISSION:
				case VTM_E_IO_CANCELED:
				case VTM_E_IO_PROTOCOL:
					continue;

				default:
					break;
			}
			return VTM_E_IO_UNKNOWN;
		}

		if (direct) {
			vtm_socket_stream_srv_sock_accepted(srv, wd, client);
		}
		else {
			node = malloc(sizeof(*node));
			if (!node) {
				vtm_err_oom();
				return vtm_err_get_code();
			}

			node->sock = client;
			node->type = VTM_SOCK_SRV_ACCEPTED;

			vtm_socket_make_threadsafe(client);
			vtm_socket_stream_srv_add_event(srv, node);
		}
	}

	VTM_ABORT_NOT_REACHABLE;
	return VTM_ERROR;
}

static int vtm_socket_stream_srv_workers_span(vtm_socket_stream_srv *srv, unsigned int threads)
{
	unsigned int i;

	srv->thread_count = threads;
	if (srv->thread_count == 0)
		return VTM_OK;

	srv->threads = calloc(srv->thread_count, sizeof(vtm_thread*));
	if (!srv->threads)
		return VTM_ERROR;

	for (i=0; i < srv->thread_count; i++) {
		srv->threads[i] = vtm_thread_new(vtm_socket_stream_srv_worker_run, srv);
		if (!srv->threads[i])
			return VTM_ERROR;
	}

	return VTM_OK;
}

static void vtm_socket_stream_srv_workers_interrupt(vtm_socket_stream_srv *srv)
{
	if (!srv->events_cond)
		return;

	vtm_cond_signal_all(srv->events_cond);
}

static void vtm_socket_stream_srv_workers_join(vtm_socket_stream_srv *srv)
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

static void vtm_socket_stream_srv_workers_free(vtm_socket_stream_srv *srv)
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

static int vtm_socket_stream_srv_worker_run(void *arg)
{
	vtm_dataset *wd;
	struct vtm_socket_stream_srv_entry *event;
	vtm_socket_stream_srv *srv;
	bool event_processed;

	srv = arg;
	wd = vtm_dataset_new();
	if (!wd)
		return vtm_err_get_code();

	if (srv->cbs.worker_init)
		srv->cbs.worker_init(srv, wd);

	/* normal operation */
	while (vtm_atomic_flag_isset(srv->running)) {
		vtm_mutex_lock(srv->events_mtx);

		while (VTM_SQUEUE_IS_EMPTY(srv->events)) {
			if (!vtm_atomic_flag_isset(srv->running)) {
				vtm_mutex_unlock(srv->events_mtx);
				goto finish;
			}

			vtm_cond_wait(srv->events_cond, srv->events_mtx);
		}

		VTM_SQUEUE_POLL(srv->events, event);
		vtm_mutex_unlock(srv->events_mtx);

		event_processed = vtm_socket_stream_srv_sock_event(srv, wd, event);
		vtm_socket_unref(event->sock);

		if (event_processed)
			free(event);
	}

finish:

	/* wait for draining */
	vtm_latch_count(&srv->drain_prepare_latch);
	vtm_latch_await(&srv->drain_run_latch);

	/* draining */
	while (true) {
		vtm_mutex_lock(srv->events_mtx);
		VTM_SQUEUE_POLL(srv->events, event);
		vtm_mutex_unlock(srv->events_mtx);

		if (!event)
			break;

		event_processed = vtm_socket_stream_srv_sock_event(srv, wd, event);
		if (event_processed)
			free(event);
	}

	if (srv->cbs.worker_end)
		srv->cbs.worker_end(srv, wd);

	vtm_dataset_free(wd);

	return VTM_OK;
}

static VTM_INLINE bool vtm_socket_stream_srv_sock_event(vtm_socket_stream_srv *srv, vtm_dataset *wd, struct vtm_socket_stream_srv_entry *event)
{
	int rc;

	switch (event->type) {
		case VTM_SOCK_SRV_ACCEPTED:
			rc = vtm_socket_stream_srv_sock_trylock(event->sock, VTM_SOCK_STAT_READ_LOCKED);
			VTM_ASSERT(rc == VTM_OK);
			vtm_socket_stream_srv_sock_accepted(srv, wd, event->sock);
			vtm_socket_stream_srv_sock_unlock(event->sock, VTM_SOCK_STAT_READ_LOCKED);
			break;

		case VTM_SOCK_SRV_CLOSED:
			rc = vtm_socket_stream_srv_sock_trylock(event->sock,
				VTM_SOCK_STAT_READ_LOCKED | VTM_SOCK_STAT_WRITE_LOCKED);
			if (rc != VTM_OK) {
				vtm_socket_stream_srv_add_event(srv, event);
				return false;
			}
			vtm_socket_stream_srv_sock_closed(srv, wd, event->sock);
			vtm_socket_stream_srv_sock_unlock(event->sock,
				VTM_SOCK_STAT_READ_LOCKED | VTM_SOCK_STAT_WRITE_LOCKED);
			break;

		case VTM_SOCK_SRV_ERROR:
			rc = vtm_socket_stream_srv_sock_trylock(event->sock,
				VTM_SOCK_STAT_READ_LOCKED | VTM_SOCK_STAT_WRITE_LOCKED);
			if (rc != VTM_OK) {
				vtm_socket_stream_srv_add_event(srv, event);
				return false;
			}
			vtm_socket_stream_srv_sock_error(srv, wd, event->sock);
			vtm_socket_stream_srv_sock_unlock(event->sock,
				VTM_SOCK_STAT_READ_LOCKED | VTM_SOCK_STAT_WRITE_LOCKED);
			break;

		case VTM_SOCK_SRV_READ:
			if (vtm_socket_get_state(event->sock) & VTM_SOCK_STAT_CLOSED)
				return true;
			rc = vtm_socket_stream_srv_sock_trylock(event->sock, VTM_SOCK_STAT_READ_LOCKED);
			if (rc != VTM_OK)
				return true;
			vtm_socket_stream_srv_sock_can_read(srv, wd, event->sock);
			vtm_socket_stream_srv_sock_unlock(event->sock, VTM_SOCK_STAT_READ_LOCKED);
			break;

		case VTM_SOCK_SRV_WRITE:
			if (vtm_socket_get_state(event->sock) & VTM_SOCK_STAT_CLOSED)
				return true;
			rc = vtm_socket_stream_srv_sock_trylock(event->sock, VTM_SOCK_STAT_WRITE_LOCKED);
			if (rc != VTM_OK)
				return true;
			vtm_socket_stream_srv_sock_can_write(srv, wd, event->sock);
			vtm_socket_stream_srv_sock_unlock(event->sock, VTM_SOCK_STAT_WRITE_LOCKED);
			break;
	}

	return true;
}

static VTM_INLINE int vtm_socket_stream_srv_sock_check(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *sock, bool rearm)
{
	int rc;
	unsigned int state;

eval:
	state = vtm_socket_get_state(sock);
	if (state & VTM_SOCK_STAT_ERR) {
			vtm_socket_stream_srv_sock_error(srv, wd, sock);
			return VTM_E_IO_UNKNOWN;
	}
	else if (state & VTM_SOCK_STAT_CLOSED) {
			vtm_socket_stream_srv_sock_closed(srv, wd, sock);
			return VTM_E_IO_CLOSED;
	}

	if (rearm) {
		rc = vtm_socket_listener_rearm(srv->listener, sock);
		if (rc != VTM_OK) {
			vtm_socket_close(sock);
			goto eval;
		}
	}

	return VTM_OK;
}

static VTM_INLINE void vtm_socket_stream_srv_sock_accepted(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *sock)
{
	int rc;

	/* check if NONBLOCKING can be activated */
	rc = vtm_socket_set_opt(sock, VTM_SOCK_OPT_NONBLOCKING,
		(bool[]) {true}, sizeof(bool));
	if (rc != VTM_OK) {
		vtm_socket_close(sock);
		vtm_socket_stream_srv_sock_free(srv, sock);
		return;
	}

	/* set stream srv callbacks */
	vtm_socket_stream_srv_sock_init_cbs(srv, sock);

	/* run CONNECTED callback */
	if (srv->cbs.sock_connected)
		srv->cbs.sock_connected(srv, wd, sock);

	/* check if socket was closed or got error in callback */
	rc = vtm_socket_stream_srv_sock_check(srv, wd, sock, false);
	if (rc != VTM_OK)
		return;

	/* register socket to listener and add to connections */
	vtm_socket_set_state(sock, VTM_SOCK_STAT_NBL_READ);
	vtm_socket_stream_srv_lock_cons(srv);
	rc = vtm_socket_listener_add(srv->listener, sock);
	if (rc == VTM_OK)
		vtm_map_put_va(srv->cons, sock, sock);
	vtm_socket_stream_srv_unlock_cons(srv);

	/* check for error, socket listener max could be reached */
	if (rc != VTM_OK) {
		/* run DISCONNECTED callback */
		vtm_socket_close(sock);
		if (srv->cbs.sock_disconnected)
			srv->cbs.sock_disconnected(srv, wd, sock);
		vtm_socket_stream_srv_sock_free(srv, sock);
	}
}

static VTM_INLINE void vtm_socket_stream_srv_sock_can_read(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *sock)
{
	if (srv->cbs.sock_can_read)
		srv->cbs.sock_can_read(srv, wd, sock);

	vtm_socket_stream_srv_sock_check(srv, wd, sock, true);
}

static VTM_INLINE void vtm_socket_stream_srv_sock_can_write(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *sock)
{
	if (srv->cbs.sock_can_write)
		srv->cbs.sock_can_write(srv, wd, sock);

	vtm_socket_stream_srv_sock_check(srv, wd, sock, true);
}

static VTM_INLINE void vtm_socket_stream_srv_sock_closed(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *sock)
{
	vtm_socket_stream_srv_lock_cons(srv);
	vtm_map_remove_va(srv->cons, sock);
	vtm_socket_stream_srv_unlock_cons(srv);

	vtm_socket_listener_remove(srv->listener, sock);

	if (srv->cbs.sock_disconnected)
		srv->cbs.sock_disconnected(srv, wd, sock);

	vtm_socket_stream_srv_sock_free(srv, sock);
}

static VTM_INLINE void vtm_socket_stream_srv_sock_error(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *sock)
{
	if (srv->cbs.sock_error)
		srv->cbs.sock_error(srv, wd, sock);

	vtm_socket_close(sock);
	vtm_socket_stream_srv_sock_closed(srv, wd, sock);
}

static VTM_INLINE void vtm_socket_stream_srv_sock_free(vtm_socket_stream_srv *srv, vtm_socket *sock)
{
	if (srv->thread_count == 0) {
		vtm_socket_free(sock);
		return;
	}

	vtm_mutex_lock(srv->events_mtx);
	vtm_list_add_va(srv->release_socks, sock);
	vtm_mutex_unlock(srv->events_mtx);

	vtm_socket_listener_interrupt(srv->listener);
}

static void vtm_socket_stream_srv_sock_init_cbs(vtm_socket_stream_srv *srv, vtm_socket *sock)
{
	sock->stream_srv = srv;
	sock->vtm_socket_update_stream_srv = vtm_socket_stream_srv_sock_cb_update;
}

static int vtm_socket_stream_srv_sock_cb_update(void *stream_srv, vtm_socket *sock)
{
	vtm_socket_stream_srv *srv;

	srv = stream_srv;

	vtm_socket_lock(sock);

	if (sock->state & VTM_SOCK_STAT_ERR) {
		vtm_socket_ref(sock);
		vtm_socket_stream_srv_create_relay_event(srv, VTM_SOCK_SRV_ERROR, sock);
		vtm_socket_listener_interrupt(srv->listener);
	}
	else if (sock->state & VTM_SOCK_STAT_CLOSED) {
		vtm_socket_ref(sock);
		vtm_socket_stream_srv_create_relay_event(srv, VTM_SOCK_SRV_CLOSED, sock);
		vtm_socket_listener_interrupt(srv->listener);
	}
	else if (sock->state & (VTM_SOCK_STAT_READ_AGAIN | VTM_SOCK_STAT_WRITE_AGAIN)) {
		vtm_socket_listener_rearm(srv->listener, sock);
	}

	vtm_socket_unlock(sock);

	return VTM_OK;
}

static VTM_INLINE int vtm_socket_stream_srv_sock_trylock(vtm_socket *sock, unsigned int flags)
{
	int rc;

	vtm_socket_lock(sock);
	if (sock->state & flags) {
		rc = VTM_ERROR;
	}
	else {
		sock->state |= flags;
		rc = VTM_OK;
	}
	vtm_socket_unlock(sock);

	return rc;
}

static VTM_INLINE void vtm_socket_stream_srv_sock_unlock(vtm_socket *sock, unsigned int flags)
{
	vtm_socket_lock(sock);
	sock->state &= ~flags;
	vtm_socket_unlock(sock);
}

static VTM_INLINE void vtm_socket_stream_srv_lock_cons(vtm_socket_stream_srv *srv)
{
	if (srv->cons_mtx)
		vtm_mutex_lock(srv->cons_mtx);
}

static VTM_INLINE void vtm_socket_stream_srv_unlock_cons(vtm_socket_stream_srv *srv)
{
	if (srv->cons_mtx)
		vtm_mutex_unlock(srv->cons_mtx);
}

static void vtm_socket_stream_srv_free_sockets(vtm_socket_stream_srv *srv)
{
	size_t i, count;

	count = vtm_list_size(srv->release_socks);
	for (i=0; i < count; i++)
		vtm_socket_free(vtm_list_get_pointer(srv->release_socks, i));
}
