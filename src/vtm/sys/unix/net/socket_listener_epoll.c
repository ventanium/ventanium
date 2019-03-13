/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/net/socket_listener.h>

#include <stdlib.h> /* malloc() */
#include <string.h> /* memset() */
#include <errno.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h> /* close() */

#include <vtm/core/error.h>
#include <vtm/net/socket_intl.h>

struct vtm_socket_listener
{
	int efd;
	int cfd;
	struct epoll_event *events;
	struct vtm_socket_event *sock_events;
	int num_events;
};

/* forward declaration */
static int vtm_socket_listener_add_closer(vtm_socket_listener *li);
static int vtm_socket_listener_epoll_ctl(vtm_socket_listener *li, vtm_socket *sock, int op);
static void vtm_socket_listener_epoll_fill(vtm_socket *sock, struct epoll_event *event);

vtm_socket_listener* vtm_socket_listener_new(size_t max_events)
{
	vtm_socket_listener *li;

	if (max_events > INT_MAX) {
		vtm_err_set(VTM_E_INVALID_ARG);
		return NULL;
	}

	li = malloc(sizeof(vtm_socket_listener));
	if (!li) {
		vtm_err_oom();
		return NULL;
	}

	li->efd = epoll_create1(0);
	if (li->efd < 0)
		goto err;

	li->cfd = eventfd(0, EFD_NONBLOCK);
	if (li->cfd < 0)
		goto err_efd;

	if (vtm_socket_listener_add_closer(li) != VTM_OK)
		goto err_cfd;

	li->events = calloc(max_events, sizeof(struct epoll_event));
	if (!li->events)
		goto err_cfd;

	li->sock_events = calloc(max_events, sizeof(struct vtm_socket_event));
	if (!li->sock_events)
		goto err_events;

	li->num_events = max_events;

	return li;

err_events:
	free(li->events);

err_cfd:
	close(li->cfd);

err_efd:
	close(li->efd);

err:
	free(li);

	return NULL;
}

void vtm_socket_listener_free(vtm_socket_listener *li)
{
	close(li->cfd);
	close(li->efd);
	free(li->sock_events);
	free(li->events);
	free(li);
}

static int vtm_socket_listener_add_closer(vtm_socket_listener *li)
{
	int rc;
	struct epoll_event event;

	memset(&event, 0, sizeof(event));
	event.data.ptr = li;
	event.events = EPOLLET | EPOLLIN;
	rc = epoll_ctl(li->efd, EPOLL_CTL_ADD, li->cfd, &event);
	if (rc < 0)
		return VTM_ERROR;

	return VTM_OK;
}

int vtm_socket_listener_add(vtm_socket_listener *li, vtm_socket *sock)
{
	return vtm_socket_listener_epoll_ctl(li, sock, EPOLL_CTL_ADD);
}

int vtm_socket_listener_remove(vtm_socket_listener *li, vtm_socket *sock)
{
	int rc;

	vtm_socket_lock(sock);

	if ((vtm_socket_get_state(sock) & VTM_SOCK_STAT_CLOSED) == 0) {
		rc = epoll_ctl(li->efd, EPOLL_CTL_DEL, VTM_SOCK_FD(sock), NULL);
		rc = rc == 0 ? VTM_OK : VTM_ERROR;
	}
	else {
		rc = VTM_OK;
	}

	vtm_socket_unlock(sock);

	return rc;
}

int vtm_socket_listener_rearm(vtm_socket_listener *li, vtm_socket *sock)
{
	return vtm_socket_listener_epoll_ctl(li, sock, EPOLL_CTL_MOD);
}

int vtm_socket_listener_run(vtm_socket_listener *li, struct vtm_socket_event **events, size_t *num_events)
{
	int i, n, off;
	unsigned int types;
	uint64_t buf;

	off = 0;
	n = epoll_wait(li->efd, li->events, li->num_events, -1);
	for (i=0; i < n; i++) {
		/* li->cfd has data to read, listener was interrupted */
		if (li->events[i].data.ptr == li) {
			read(li->cfd, &buf, sizeof(uint64_t));
			off = 1;
			continue;
		}

		types = 0;
		if ((li->events[i].events & EPOLLHUP) ||
			(li->events[i].events & EPOLLRDHUP)) {
			types = VTM_SOCK_EVT_CLOSED;
		}
		if (li->events[i].events & EPOLLIN) {
			types |= VTM_SOCK_EVT_READ;
		}
		if (li->events[i].events & EPOLLOUT) {
			types |= VTM_SOCK_EVT_WRITE;
		}
		if (types == 0) {
			types = VTM_SOCK_EVT_ERROR;
		}

		li->sock_events[i-off].sock = (vtm_socket*) li->events[i].data.ptr;
		li->sock_events[i-off].events = types;
	}

	*events = li->sock_events;
	*num_events = n > 0 ? n - off : 0;

	return VTM_OK;
}

int vtm_socket_listener_interrupt(vtm_socket_listener *li)
{
	write(li->cfd, (uint64_t[]) {1}, sizeof(uint64_t));
	return VTM_OK;
}

static int vtm_socket_listener_epoll_ctl(vtm_socket_listener *li, vtm_socket *sock, int op)
{
	int rc;
	struct epoll_event event;

	memset(&event, 0, sizeof(event));
	event.data.ptr = sock;
	event.events = EPOLLONESHOT;

	vtm_socket_lock(sock);

	vtm_socket_listener_epoll_fill(sock, &event);
	rc = epoll_ctl(li->efd, op, VTM_SOCK_FD(sock), &event);
	if (rc < 0)
		rc = (errno == ENOSPC) ? VTM_E_MAX_REACHED : VTM_ERROR;
	else
		rc = VTM_OK;

	vtm_socket_unlock(sock);

	return rc;
}

static void vtm_socket_listener_epoll_fill(vtm_socket *sock, struct epoll_event *event)
{
	if ((sock->state & VTM_SOCK_STAT_NBL_READ) != 0)
		event->events |= EPOLLIN;

	if ((sock->state & VTM_SOCK_STAT_NBL_WRITE) != 0)
		event->events |= EPOLLOUT;
}
