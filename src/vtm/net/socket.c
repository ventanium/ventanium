/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "socket.h"

#include <vtm/core/error.h>
#include <vtm/core/flag.h>
#include <vtm/core/lang.h>
#include <vtm/net/socket_intl.h>

#define VTM_SOCKET_IS_CLOSED(SOCK)      \
	vtm_flag_is_set((SOCK)->state, VTM_SOCK_STAT_CLOSED)

int vtm_socket_base_init(struct vtm_socket *sock)
{
	sock->state = VTM_SOCK_STAT_DEFAULT;
	sock->mtx = NULL;
	sock->usr_data = NULL;
	sock->refcount = 0;

	return VTM_OK;
}

void vtm_socket_free(vtm_socket *sock)
{
	if (!sock)
		return;

	vtm_mutex_free(sock->mtx);
	sock->vtable->vtm_socket_free(sock);
}

int vtm_socket_make_threadsafe(vtm_socket *sock)
{
	if (sock->mtx)
		return VTM_E_INVALID_STATE;

	sock->mtx = vtm_mutex_new();
	if (!sock->mtx)
		return vtm_err_get_code();

	return VTM_OK;
}

void vtm_socket_set_usr_data(vtm_socket *sock, void *data)
{
	vtm_socket_lock(sock);
	sock->usr_data = data;
	vtm_socket_unlock(sock);
}

void* vtm_socket_get_usr_data(vtm_socket *sock)
{
	void *data;

	vtm_socket_lock(sock);
	data = sock->usr_data;
	vtm_socket_unlock(sock);

	return data;
}

enum vtm_socket_family vtm_socket_get_family(vtm_socket *sock)
{
	return sock->family;
}

int vtm_socket_get_type(vtm_socket *sock)
{
	return sock->type;
}

int vtm_socket_bind(vtm_socket *sock, const char *addr, unsigned int port)
{
	int rc;

	vtm_socket_lock(sock);
	if (VTM_SOCKET_IS_CLOSED(sock))
		rc = VTM_E_IO_CLOSED;
	else
		rc = sock->vtable->vtm_socket_bind(sock, addr, port);
	vtm_socket_unlock(sock);

	return rc;
}

int vtm_socket_listen(vtm_socket *sock, unsigned int backlog)
{
	int rc;

	vtm_socket_lock(sock);
	if (VTM_SOCKET_IS_CLOSED(sock))
		rc = VTM_E_IO_CLOSED;
	else
		rc = sock->vtable->vtm_socket_listen(sock, backlog);
	vtm_socket_unlock(sock);

	return rc;
}

int vtm_socket_accept(vtm_socket *sock, vtm_socket **client)
{
	int rc;

	vtm_socket_lock(sock);
	if (VTM_SOCKET_IS_CLOSED(sock)) {
		rc = VTM_E_IO_CLOSED;
	}
	else {
		vtm_flag_unset(sock->state, VTM_SOCK_STAT_READ_AGAIN);
		rc = sock->vtable->vtm_socket_accept(sock, client);
	}
	vtm_socket_unlock(sock);

	return rc;
}

int vtm_socket_connect(vtm_socket *sock, const char *host, unsigned int port)
{
	int rc;

	vtm_socket_lock(sock);
	if (VTM_SOCKET_IS_CLOSED(sock)) {
		rc = VTM_E_IO_CLOSED;
	}
	else {
		vtm_flag_unset(sock->state, VTM_SOCK_STAT_READ_AGAIN);
		rc = sock->vtable->vtm_socket_connect(sock, host, port);
	}
	vtm_socket_unlock(sock);

	return rc;
}

int vtm_socket_shutdown(vtm_socket *sock, int dir)
{
	int rc;

	vtm_socket_lock(sock);
	if (VTM_SOCKET_IS_CLOSED(sock))
		rc = VTM_E_IO_CLOSED;
	else
		rc = sock->vtable->vtm_socket_shutdown(sock, dir);
	vtm_socket_unlock(sock);

	return rc;
}

int vtm_socket_close(vtm_socket *sock)
{
	int rc;

	vtm_socket_lock(sock);
	if (VTM_SOCKET_IS_CLOSED(sock)) {
		rc = VTM_OK;
	}
	else {
		rc = sock->vtable->vtm_socket_close(sock);
		if (rc == VTM_OK)
			vtm_flag_set(sock->state, VTM_SOCK_STAT_CLOSED);
	}
	vtm_socket_unlock(sock);

	return rc;
}

int vtm_socket_write(vtm_socket *sock, const void *src, size_t len, size_t *out_written)
{
	int rc;

	vtm_socket_lock(sock);
	if (VTM_SOCKET_IS_CLOSED(sock)) {
		*out_written = 0;
		rc = VTM_E_IO_CLOSED;
		goto unlock;
	}

	vtm_flag_unset(sock->state, VTM_SOCK_STAT_WRITE_AGAIN |
		VTM_SOCK_STAT_WRITE_AGAIN_WHEN_READABLE);
	rc = sock->vtable->vtm_socket_write(sock, src, len, out_written);

	/* check if NBL hints must be changed */
	if (sock->state & VTM_SOCK_STAT_NBL_AUTO) {
		if (sock->state & (VTM_SOCK_STAT_WRITE_AGAIN |
			VTM_SOCK_STAT_READ_AGAIN_WHEN_WRITEABLE)) {
			sock->state &= ~VTM_SOCK_STAT_NBL_READ;
			sock->state |= VTM_SOCK_STAT_NBL_WRITE;
		}
		else {
			sock->state &= ~VTM_SOCK_STAT_NBL_WRITE;
			sock->state |= VTM_SOCK_STAT_NBL_READ;
		}
	}

unlock:
	vtm_socket_unlock(sock);

	return rc;
}

int vtm_socket_read(vtm_socket *sock, void *buf, size_t len, size_t *out_read)
{
	int rc;

	vtm_socket_lock(sock);
	if (VTM_SOCKET_IS_CLOSED(sock)) {
		*out_read = 0;
		rc = VTM_E_IO_CLOSED;
		goto unlock;
	}

	vtm_flag_unset(sock->state, VTM_SOCK_STAT_READ_AGAIN |
		VTM_SOCK_STAT_READ_AGAIN_WHEN_WRITEABLE);
	rc = sock->vtable->vtm_socket_read(sock, buf, len, out_read);

	/* check if NBL hints must be changed */
	if ((sock->state & VTM_SOCK_STAT_NBL_AUTO) &&
		(sock->state & VTM_SOCK_STAT_READ_AGAIN_WHEN_WRITEABLE)) {
		sock->state &= ~VTM_SOCK_STAT_NBL_READ;
		sock->state |= VTM_SOCK_STAT_NBL_WRITE;
	}

unlock:
	vtm_socket_unlock(sock);

	return rc;
}

int vtm_socket_dgram_recv(vtm_socket *sock, void *buf, size_t maxlen, size_t *out_recv, struct vtm_socket_saddr *saddr)
{
	int rc;

	vtm_socket_lock(sock);
	if (VTM_SOCKET_IS_CLOSED(sock))
		rc = VTM_E_IO_CLOSED;
	else
		rc = sock->vtable->vtm_socket_dgram_recv(sock, buf, maxlen, out_recv, saddr);
	vtm_socket_unlock(sock);

	return rc;
}

int vtm_socket_dgram_send(vtm_socket *sock, const void *buf, size_t len, size_t *out_send, const struct vtm_socket_saddr *saddr)
{
	int rc;

	vtm_socket_lock(sock);
	if (VTM_SOCKET_IS_CLOSED(sock))
		rc = VTM_E_IO_CLOSED;
	else
		rc = sock->vtable->vtm_socket_dgram_send(sock, buf, len, out_send, saddr);
	vtm_socket_unlock(sock);

	return rc;
}

void vtm_socket_set_state(vtm_socket *sock, unsigned int flags)
{
	vtm_socket_lock(sock);
	vtm_flag_set(sock->state, flags);
	vtm_socket_unlock(sock);
}

void vtm_socket_unset_state(vtm_socket *sock, unsigned int flags)
{
	vtm_socket_lock(sock);
	vtm_flag_unset(sock->state, flags);
	vtm_socket_unlock(sock);
}

unsigned int vtm_socket_get_state(vtm_socket *sock)
{
	unsigned int flags;

	vtm_socket_lock(sock);
	flags = sock->state;
	vtm_socket_unlock(sock);

	return flags;
}

int vtm_socket_set_opt(vtm_socket *sock, int opt, const void *val, size_t len)
{
	int rc;

	vtm_socket_lock(sock);

	rc = sock->vtable->vtm_socket_set_opt(sock, opt, val, len);
	if (rc != VTM_OK)
		goto unlock;

	switch (opt) {
		case VTM_SOCK_OPT_NONBLOCKING:
			if (val)
				vtm_flag_set(sock->state, VTM_SOCK_STAT_NONBLOCKING);
			else
				vtm_flag_unset(sock->state, VTM_SOCK_STAT_NONBLOCKING);
			break;

		default:
			break;
	}
unlock:
	vtm_socket_unlock(sock);

	return rc;
}

int vtm_socket_get_opt(vtm_socket *sock, int opt, void *val, size_t len)
{
	int rc;

	vtm_socket_lock(sock);

	switch (opt) {
		case VTM_SOCK_OPT_NONBLOCKING:
			if (len != sizeof(bool)) {
				rc = VTM_E_INVALID_ARG;
				goto unlock;
			}
			*((bool*)val) = sock->state & VTM_SOCK_STAT_NONBLOCKING;
			rc = VTM_OK;
			goto unlock;

		default:
			break;
	}

	rc = sock->vtable->vtm_socket_get_opt(sock, opt, val, len);

unlock:
	vtm_socket_unlock(sock);

	return rc;
}

int vtm_socket_get_remote_addr(vtm_socket *sock, struct vtm_socket_saddr *addr)
{
	int rc;

	vtm_socket_lock(sock);
	rc = sock->vtable->vtm_socket_get_remote_addr(sock, addr);
	vtm_socket_unlock(sock);

	return rc;
}

VTM_INLINE void vtm_socket_lock(vtm_socket *sock)
{
	if (sock->mtx)
		vtm_mutex_lock(sock->mtx);
}

VTM_INLINE void vtm_socket_unlock(vtm_socket *sock)
{
	if (sock->mtx)
		vtm_mutex_unlock(sock->mtx);
}

void vtm_socket_ref(struct vtm_socket *sock)
{
	vtm_socket_lock(sock);
	VTM_ASSERT(sock->refcount != UINT8_MAX);
	sock->refcount++;
	vtm_socket_unlock(sock);
}

void vtm_socket_unref(struct vtm_socket *sock)
{
	bool release;

	vtm_socket_lock(sock);
	VTM_ASSERT(sock->refcount > 0);
	sock->refcount--;
	release = sock->refcount == 0 && (sock->state & VTM_SOCK_STAT_FREE_ON_UNREF);
	vtm_socket_unlock(sock);

	if (release)
		vtm_socket_free(sock);
}

void vtm_socket_enable_free_on_unref(struct vtm_socket *sock)
{
	bool release;

	vtm_socket_lock(sock);
	sock->state |= VTM_SOCK_STAT_FREE_ON_UNREF;
	release = sock->refcount == 0;
	vtm_socket_unlock(sock);

	if (release)
		vtm_socket_free(sock);
}

int vtm_socket_update_srv(struct vtm_socket *sock)
{
	if (sock->vtm_socket_update_stream_srv)
		return sock->vtm_socket_update_stream_srv(sock->stream_srv, sock);

	return VTM_E_NOT_SUPPORTED;
}
