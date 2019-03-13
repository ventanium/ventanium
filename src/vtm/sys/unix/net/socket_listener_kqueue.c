/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/net/socket_listener.h>

#include <stdlib.h> /* malloc() */
#include <string.h> /* memset() */
#include <unistd.h> /* close(), pipe() */
#include <fcntl.h> /* O_NONBLOCK */

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include <vtm/core/error.h>
#include <vtm/net/socket_intl.h>

struct vtm_socket_listener
{
	int kq;
	int cfd[2];
	struct kevent *events;
	struct vtm_socket_event *sock_events;
	int num_events;
};

/* forward declaration */
static int vtm_socket_listener_add_closer(vtm_socket_listener *li);
static int vtm_socket_listener_kevent_fill(vtm_socket_listener *li, vtm_socket *sock);

vtm_socket_listener* vtm_socket_listener_new(size_t max_events)
{
	vtm_socket_listener *li;
#ifndef VTM_SYS_BSD
	int flags;
#endif

	if (max_events > INT_MAX) {
		vtm_err_set(VTM_E_INVALID_ARG);
		return NULL;
	}

	li = malloc(sizeof(vtm_socket_listener));
	if (!li) {
		vtm_err_oom();
		return NULL;
	}

	li->kq = kqueue();
	if (li->kq < 0)
		goto err;

#ifdef VTM_SYS_BSD
	if (pipe2(li->cfd, O_NONBLOCK) < 0)
		goto err_kq;
#else
	if (pipe(li->cfd) < 0)
		goto err_kq;

	flags = fcntl(li->cfd[0], F_GETFL, 0);
	if (flags < 0)
		goto err_kq;

	if (fcntl(li->cfd[0], F_SETFL, flags | O_NONBLOCK) < 0)
		goto err_kq;
#endif

	if (vtm_socket_listener_add_closer(li) != VTM_OK)
		goto err_cfd;

	li->events = calloc(max_events, sizeof(struct kevent));
	if (!li->events) {
		vtm_err_oom();
		goto err_cfd;
	}

	li->sock_events = calloc(max_events, sizeof(struct vtm_socket_event));
	if (!li->sock_events) {
		vtm_err_oom();
		goto err_events;
	}

	li->num_events = max_events;

	return li;

err_events:
	free(li->events);

err_kq:
	close(li->kq);

err_cfd:
	close(li->cfd[0]);
	close(li->cfd[1]);

err:
	free(li);

	return NULL;
}

void vtm_socket_listener_free(vtm_socket_listener *li)
{
	close(li->cfd[0]);
	close(li->cfd[1]);
	close(li->kq);
	free(li->sock_events);
	free(li->events);
	free(li);
}

static int vtm_socket_listener_add_closer(vtm_socket_listener *li)
{
	int rc;
	struct kevent event;

	EV_SET(&event, li->cfd[0], EVFILT_READ, EV_ADD, 0, 0, li);
	rc = kevent(li->kq, &event, 1, NULL, 0, NULL);
	if (rc < 0)
		return VTM_ERROR;

	return VTM_OK;
}

int vtm_socket_listener_add(vtm_socket_listener *li, vtm_socket *sock)
{
	return vtm_socket_listener_kevent_fill(li, sock);
}

int vtm_socket_listener_remove(vtm_socket_listener *li, vtm_socket *sock)
{
	int rc;
	struct kevent events[2];

	vtm_socket_lock(sock);

	if ((vtm_socket_get_state(sock) & VTM_SOCK_STAT_CLOSED) == 0) {
		EV_SET(&events[0], VTM_SOCK_FD(sock), EVFILT_READ, EV_DELETE, 0, 0, NULL);
		EV_SET(&events[1], VTM_SOCK_FD(sock), EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
		rc = kevent(li->kq, &events[0], 2, NULL, 0, NULL);
		rc = rc != -1 ? VTM_OK : VTM_ERROR;
	}
	else {
		rc = VTM_OK;
	}

	vtm_socket_unlock(sock);

	return rc;
}

int vtm_socket_listener_rearm(vtm_socket_listener *li, vtm_socket *sock)
{
	return vtm_socket_listener_kevent_fill(li, sock);
}

int vtm_socket_listener_run(vtm_socket_listener *li, struct vtm_socket_event **events, size_t *num_events)
{
	int i, n, off;
	unsigned int types;
	char buf;

	off = 0;
	n = kevent(li->kq, NULL, 0, li->events, li->num_events, NULL);
	for (i=0; i < n; i++) {
		/* li->cfd has data to read, listener was interrupted */
		if (li->events[i].udata == li) {
			read(li->cfd[1], &buf, sizeof(char));
			off = 1;
			continue;
		}

		types = 0;
		if (li->events[i].flags & EV_EOF) {
			types = VTM_SOCK_EVT_CLOSED;
		}
		if (li->events[i].filter == EVFILT_READ) {
			types |= VTM_SOCK_EVT_READ;
		}
		if (li->events[i].filter == EVFILT_WRITE) {
			types |= VTM_SOCK_EVT_WRITE;
		}
		if (types == 0) {
			types = VTM_SOCK_EVT_ERROR;
		}

		li->sock_events[i-off].sock = (vtm_socket*) li->events[i].udata;
		li->sock_events[i-off].events = types;
	}

	*events = li->sock_events;
	*num_events = n > 0 ? n - off : 0;

	return VTM_OK;
}

int vtm_socket_listener_interrupt(vtm_socket_listener *li)
{
	write(li->cfd[1], (char[]) {1}, sizeof(char));
	return VTM_OK;
}

static int vtm_socket_listener_kevent_fill(vtm_socket_listener *li, vtm_socket *sock)
{
	int rc, count;
	struct kevent events[2];

	count = 0;
	vtm_socket_lock(sock);

	if ((sock->state & VTM_SOCK_STAT_NBL_READ) != 0) {
		EV_SET(&events[count], VTM_SOCK_FD(sock), EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, sock);
		count++;
	}

	if ((sock->state & VTM_SOCK_STAT_NBL_WRITE) != 0) {
		EV_SET(&events[count], VTM_SOCK_FD(sock), EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, sock);
		count++;
	}

	if (count > 0) {
		rc = kevent(li->kq, &events[0], count, NULL, 0, NULL);
		rc = (rc < 0) ? VTM_ERROR : VTM_OK;
	}
	else {
		rc = VTM_OK;
	}

	vtm_socket_unlock(sock);

	return rc;
}
