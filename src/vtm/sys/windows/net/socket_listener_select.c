/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/net/socket_listener.h>

#include <stdlib.h> /* malloc() */
#include <string.h> /* memset() */
#include <winsock2.h>

#include <vtm/core/error.h>
#include <vtm/core/map.h>
#include <vtm/core/math.h>
#include <vtm/net/socket_intl.h>
#include <vtm/util/mutex.h>

struct vtm_socket_listener
{
	fd_set write_set;
	fd_set read_set;

	fd_set write_buf_set;
	fd_set read_buf_set;

	unsigned int read_buf_cnt;
	unsigned int write_buf_cnt;

	struct vtm_socket_event *sock_events;
	size_t num_events;

	vtm_mutex *mapping_mtx;
	vtm_map *mapping;
	unsigned int num_sockets;
};

/* forward declaration */
static void vtm_socket_listener_select_fill(vtm_socket_listener *li, vtm_socket *sock);

vtm_socket_listener* vtm_socket_listener_new(size_t max_events)
{
	vtm_socket_listener *li;

	if (max_events > FD_SETSIZE) {
		vtm_err_set(VTM_E_INVALID_ARG);
		return NULL;
	}

	li = malloc(sizeof(vtm_socket_listener));
	if (!li) {
		vtm_err_oom();
		return NULL;
	}

	li->sock_events = calloc(max_events, sizeof(struct vtm_socket_event));
	if (!li->sock_events)
		goto err;

	li->mapping = vtm_map_new(VTM_ELEM_ULONG, VTM_ELEM_POINTER, 16);
	if (!li->mapping)
		goto err_events;

	li->mapping_mtx = vtm_mutex_new();
	if (!li->mapping_mtx)
		goto err_mapping;

	FD_ZERO(&li->write_set);
	FD_ZERO(&li->read_set);

	FD_ZERO(&li->write_buf_set);
	FD_ZERO(&li->read_buf_set);

	li->write_buf_cnt = 0;
	li->read_buf_cnt = 0;
	li->num_events = max_events;
	li->num_sockets = 0;

	return li;

err_mapping:
	vtm_map_free(li->mapping);

err_events:
	free(li->sock_events);

err:
	free(li);

	return NULL;
}

void vtm_socket_listener_free(vtm_socket_listener *li)
{
	vtm_mutex_free(li->mapping_mtx);
	vtm_map_free(li->mapping);
	free(li->sock_events);
	free(li);
}

int vtm_socket_listener_add(vtm_socket_listener *li, vtm_socket *sock)
{
	int rc;

	rc = VTM_OK;

	vtm_mutex_lock(li->mapping_mtx);

	if (li->num_sockets < FD_SETSIZE) {
		li->num_sockets++;
		vtm_socket_listener_select_fill(li, sock);
	}
	else {
		rc = VTM_E_MAX_REACHED;
	}

	vtm_mutex_unlock(li->mapping_mtx);

	return rc;
}

int vtm_socket_listener_remove(vtm_socket_listener *li, vtm_socket *sock)
{
	vtm_mutex_lock(li->mapping_mtx);
	li->num_sockets--;

	FD_CLR(VTM_SOCK_FD(sock), &li->write_set);
	FD_CLR(VTM_SOCK_FD(sock), &li->read_set);

	if (FD_ISSET(VTM_SOCK_FD(sock), &li->write_buf_set)) {
		FD_CLR(VTM_SOCK_FD(sock), &li->write_buf_set);
		li->write_buf_cnt--;
	}

	if (FD_ISSET(VTM_SOCK_FD(sock), &li->read_buf_set)) {
		FD_CLR(VTM_SOCK_FD(sock), &li->read_buf_set);
		li->read_buf_cnt--;
	}

	vtm_map_remove_va(li->mapping, VTM_SOCK_FD(sock));
	vtm_mutex_unlock(li->mapping_mtx);

	return VTM_OK;
}

int vtm_socket_listener_rearm(vtm_socket_listener *li, vtm_socket *sock)
{
	vtm_mutex_lock(li->mapping_mtx);
	vtm_socket_listener_select_fill(li, sock);
	vtm_mutex_unlock(li->mapping_mtx);

	return VTM_OK;
}

static void vtm_socket_listener_select_fill(vtm_socket_listener *li, vtm_socket *sock)
{
	vtm_socket_lock(sock);

	if ((sock->state & VTM_SOCK_STAT_NBL_READ) != 0)
		FD_SET(VTM_SOCK_FD(sock), &li->read_set);

	if ((sock->state & VTM_SOCK_STAT_NBL_WRITE) != 0)
		FD_SET(VTM_SOCK_FD(sock), &li->write_set);

	vtm_socket_unlock(sock);

	vtm_map_put_va(li->mapping, VTM_SOCK_FD(sock), sock);
}

int vtm_socket_listener_run(vtm_socket_listener *li, struct vtm_socket_event **events, size_t *num_events)
{
	int rc, n;
	fd_set read_set, write_set;
	vtm_socket *sock;
	vtm_list *entries;
	struct vtm_map_entry *entry;
	SOCKET fd;
	size_t i, entries_count, event_idx;
	struct timeval tv;
	bool buffered, handled;

	rc = VTM_OK;
	event_idx = 0;
	buffered = false;
	entries = NULL;

	/* check for buffered events from last call */
	vtm_mutex_lock(li->mapping_mtx);

	if (li->read_buf_cnt > 0 || li->write_buf_cnt > 0) {
		entries = vtm_map_entryset(li->mapping);
		entries_count = vtm_list_size(entries);
		for (i=0; i < entries_count; i++) {
			handled = false;
			entry = vtm_list_get_pointer(entries, i);
			sock = entry->value.elem_pointer;
			fd = VTM_SOCK_FD(sock);

			li->sock_events[event_idx].events = 0;

			if (FD_ISSET(fd, &li->read_buf_set)) {
				li->sock_events[event_idx].events = VTM_SOCK_EVT_READ;
				handled = true;
				FD_CLR(fd,  &li->read_buf_set);
				li->read_buf_cnt--;
			}

			if (FD_ISSET(fd, &li->write_buf_set)) {
				li->sock_events[event_idx].events |= VTM_SOCK_EVT_WRITE;
				handled = true;
				FD_CLR(fd,  &li->write_buf_set);
				li->write_buf_cnt--;
			}

			if (!handled)
				continue;

			li->sock_events[event_idx].sock = sock;

			vtm_map_remove_va(li->mapping, fd);
			if (++event_idx == li->num_events)
				break;
		}
		buffered = true;
	}

	if (!buffered) {
		read_set = li->read_set;
		write_set = li->write_set;
	}

	vtm_mutex_unlock(li->mapping_mtx);

	if (buffered)
		goto finish;

	/* timeout for syscall */
	tv.tv_sec = 0;
	tv.tv_usec = 2500;

	/* wait for events */
	n = select(0, &read_set, &write_set, NULL, &tv);

	/* handle special conditions */
	if (n == SOCKET_ERROR) {
		n = WSAGetLastError();
		switch (n) {
			/* all sets are empty or invalid timeout value */
			case WSAEINVAL:
				rc = VTM_OK;
				break;

			/* other error */
			default:
				rc = VTM_E_IO_UNKNOWN;
				break;
		}
		goto finish;
	}
	else if (n == 0) {
		goto finish;
	}

	/* handle events */
	vtm_mutex_lock(li->mapping_mtx);

	entries = vtm_map_entryset(li->mapping);
	entries_count = vtm_list_size(entries);

	for (i=0; i < entries_count; i++) {
		handled = false;
		entry = vtm_list_get_pointer(entries, i);
		sock = entry->value.elem_pointer;
		fd = VTM_SOCK_FD(sock);

		if (FD_ISSET(fd, &read_set)) {
			FD_CLR(fd, &li->read_set);
			if (event_idx < li->num_events) {
				li->sock_events[event_idx].events = VTM_SOCK_EVT_READ;
			}
			else {
				FD_SET(fd, &li->read_buf_set);
				li->read_buf_cnt++;
			}
			handled = true;
		}
		if (FD_ISSET(fd, &write_set)) {
			FD_CLR(fd, &li->write_set);
			if (event_idx < li->num_events) {
				li->sock_events[event_idx].events = VTM_SOCK_EVT_WRITE;
			}
			else {
				FD_SET(fd, &li->write_buf_set);
				li->write_buf_cnt++;
			}
			handled = true;
		}

		if (!handled)
			continue;

		if (event_idx < li->num_events) {
			li->sock_events[event_idx].sock = sock;
			vtm_map_remove_va(li->mapping, fd);
		}

		if (++event_idx == (size_t) n)
			break;
	}

	vtm_mutex_unlock(li->mapping_mtx);

finish:
	vtm_list_free(entries);

	*events = li->sock_events;
	*num_events = VTM_MIN(event_idx, li->num_events);

	return rc;
}

int vtm_socket_listener_interrupt(vtm_socket_listener *li)
{
	return VTM_OK;
}
