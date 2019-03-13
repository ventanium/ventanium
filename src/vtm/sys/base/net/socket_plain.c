/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include <vtm/net/socket.h>

#include <string.h> /* memset() */
#include <errno.h> /* errno */

#ifdef VTM_HAVE_POSIX

	#include <netinet/in.h> /* sockaddr_in */
	#include <unistd.h> /* close() */

	#include <sys/types.h>
	#include <sys/socket.h>

	#define VTM_SHUT_RD                SHUT_RD
	#define VTM_SHUT_WR                SHUT_WR
	#define VTM_SHUT_BOTH              SHUT_RDWR

	#define VTM_SOCKSIZE_CASTED(VAL)   (VAL)

#elif VTM_SYS_WINDOWS

	#include <winsock2.h>

	#define VTM_SHUT_RD                SD_RECEIVE
	#define VTM_SHUT_WR                SD_SEND
	#define VTM_SHUT_BOTH              SD_BOTH

	#define VTM_SOCKSIZE_CASTED(VAL)   ((int) (VAL))

#endif

#include <vtm/core/error.h>
#include <vtm/core/flag.h>
#include <vtm/net/socket_intl.h>
#include <vtm/sys/base/net/socket_types.h>
#include <vtm/sys/base/net/socket_types_intl.h>
#include <vtm/sys/base/net/socket_util_intl.h>

/* forward declaration */
static vtm_socket* vtm_socket_plain_alloc(enum vtm_socket_family fam, int type, vtm_sys_socket_t sockfd);
static void vtm_socket_plain_free(struct vtm_socket *sock);
static int vtm_socket_plain_accept(struct vtm_socket *sock, struct vtm_socket **client);
static int vtm_socket_plain_shutdown(struct vtm_socket *sock, int dir);
static int vtm_socket_plain_close(struct vtm_socket *sock);
static int vtm_socket_plain_write(struct vtm_socket *sock, const void *src, size_t len, size_t *out_written);
static int vtm_socket_plain_read(struct vtm_socket *sock, void *buf, size_t len, size_t *out_read);
static int vtm_socket_plain_dgram_recv(struct vtm_socket *sock, void *buf, size_t maxlen, size_t *out_recv, struct vtm_socket_saddr *saddr);
static int vtm_socket_plain_dgram_send(struct vtm_socket *sock, const void *buf, size_t len, size_t *out_send, const struct vtm_socket_saddr *saddr);

/* vtable */
static struct vtm_socket_vtable vtm_socket_plain_vtable = {
	.vtm_socket_free = vtm_socket_plain_free,
	.vtm_socket_bind = vtm_socket_util_bind,
	.vtm_socket_listen = vtm_socket_util_listen,
	.vtm_socket_accept = vtm_socket_plain_accept,
	.vtm_socket_connect = vtm_socket_util_connect,
	.vtm_socket_shutdown = vtm_socket_plain_shutdown,
	.vtm_socket_close = vtm_socket_plain_close,
	.vtm_socket_write = vtm_socket_plain_write,
	.vtm_socket_read = vtm_socket_plain_read,
	.vtm_socket_dgram_recv = vtm_socket_plain_dgram_recv,
	.vtm_socket_dgram_send = vtm_socket_plain_dgram_send,
	.vtm_socket_set_opt = vtm_socket_util_set_opt,
	.vtm_socket_get_opt = vtm_socket_util_get_opt,
	.vtm_socket_get_remote_addr = vtm_socket_util_get_remote_addr
};

vtm_socket* vtm_socket_new(enum vtm_socket_family fam, int type)
{
	int rc;
	int sockfam;
	int socktype;
	vtm_sys_socket_t sockfd;
	vtm_socket *sock;

	rc = vtm_socket_util_convert_family(fam, &sockfam);
	if (rc != VTM_OK)
		return NULL;

	rc = vtm_socket_util_convert_type(type, &socktype);
	if (rc != VTM_OK)
		return NULL;

	sockfd = socket(sockfam, socktype, 0);
	if (VTM_SOCK_INVALID(sockfd)) {
		vtm_socket_util_error(NULL);
		return NULL;
	}

	rc = vtm_socket_util_block_sigpipe(sockfd);
	if (rc != VTM_OK) {
		VTM_CLOSESOCKET(sockfd);
		return NULL;
	}

	sock = vtm_socket_plain_alloc(fam, type, sockfd);
	if (!sock)
		VTM_CLOSESOCKET(sockfd);

	return sock;
}

static vtm_socket* vtm_socket_plain_alloc(enum vtm_socket_family fam, int type, vtm_sys_socket_t sockfd)
{
	vtm_socket *sock;

	sock = malloc(sizeof(vtm_socket));
	if (!sock) {
		vtm_err_oom();
		return NULL;
	}

	if (vtm_socket_base_init(sock) != VTM_OK) {
		free(sock);
		return NULL;
	}

	sock->fd = sockfd;
	sock->family = fam;
	sock->type = type;
	sock->info = NULL;
	sock->vtable = &vtm_socket_plain_vtable;

	return sock;
}

static void vtm_socket_plain_free(struct vtm_socket *sock)
{
	free(sock);
}

static int vtm_socket_plain_accept(struct vtm_socket *sock, struct vtm_socket **client)
{
	int rc;
	vtm_sys_socket_t sockfd;
	struct sockaddr_in client_addr;
	vtm_sys_socklen_t length;
	struct vtm_socket *out;

	rc = VTM_OK;
	length = sizeof(client_addr);

	sockfd = accept(sock->fd, (struct sockaddr*) &client_addr, &length);
	if (VTM_SOCK_INVALID(sockfd))
		return vtm_socket_util_read_error(sock);

#if defined(VTM_SYS_BSD) || defined(VTM_SYS_DARWIN)
	/* on BSD sockets inherit nonblocking state */
	bool nbl;
	rc = vtm_socket_get_opt(sock, VTM_SOCK_OPT_NONBLOCKING, &nbl, sizeof(bool));
	if (rc == VTM_OK && nbl)
		rc = vtm_socket_util_set_nonblocking(sockfd, false);
	if (rc != VTM_OK) {
		VTM_CLOSESOCKET(sockfd);
		return rc;
	}
#endif

	out = vtm_socket_plain_alloc(sock->family, sock->type, sockfd);
	if (!out) {
		VTM_CLOSESOCKET(sockfd);
		rc = VTM_ERROR;
	}

	*client = out;

	return rc;
}

static int vtm_socket_plain_shutdown(struct vtm_socket *sock, int dir)
{
	int rc;
	int how;

	if (vtm_flag_is_set(dir, VTM_SOCK_SHUT_RD | VTM_SOCK_SHUT_WR))
		how = VTM_SHUT_BOTH;
	else if (vtm_flag_is_set(dir, VTM_SOCK_SHUT_RD))
		how = VTM_SHUT_RD;
	else if (vtm_flag_is_set(dir, VTM_SOCK_SHUT_WR))
		how = VTM_SHUT_WR;
	else
		return VTM_E_INVALID_ARG;

	rc = shutdown(sock->fd, how);
	if (VTM_SOCK_ERR(rc))
		return vtm_socket_util_error(sock);

	return VTM_OK;
}

static int vtm_socket_plain_close(struct vtm_socket *sock)
{
	VTM_CLOSESOCKET(sock->fd);

	return VTM_OK;
}

static int vtm_socket_plain_write(struct vtm_socket *sock, const void *src, size_t len, size_t *out_written)
{
	int rc;
	size_t written;
	vtm_sys_sockrc_t num;

#ifdef VTM_SYS_WINDOWS
	if (len > INT_MAX)
		return vtm_err_set(VTM_E_INVALID_ARG);
#endif

	rc = VTM_OK;

	written = 0;
	while (written != len) {
		num = send(sock->fd, (const char*) src + written, VTM_SOCKSIZE_CASTED(len - written), 0);
		if (num < 0) {
#ifdef VTM_SYS_UNIX
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
#elif VTM_SYS_WINDOWS
			if (WSAGetLastError() == WSAEWOULDBLOCK) {
#endif
				rc = VTM_E_IO_AGAIN;
				vtm_socket_set_state_intl(sock, VTM_SOCK_STAT_WRITE_AGAIN);
				goto out;
			}
			vtm_socket_set_state_intl(sock, VTM_SOCK_STAT_ERR);
			return vtm_err_set(VTM_E_IO_UNKNOWN);
		}
		written += num;
	}

out:
	*out_written = written;
	return rc;
}

static int vtm_socket_plain_read(struct vtm_socket *sock, void *buf, size_t len, size_t *out_read)
{
	int rc;
	unsigned int state;
	vtm_sys_sockrc_t num;

#ifdef VTM_SYS_WINDOWS
	if (len > INT_MAX)
		len = INT_MAX;
#endif

	num = recv(sock->fd, buf, VTM_SOCKSIZE_CASTED(len), 0);
	if (num < 0) {
		rc = vtm_socket_util_error(NULL);
		state = (rc == VTM_E_IO_AGAIN) ? VTM_SOCK_STAT_READ_AGAIN : VTM_SOCK_STAT_ERR;
		num = 0;
	}
	else if (num == 0) {
		rc = VTM_E_IO_CLOSED;
		state = VTM_SOCK_STAT_CLOSED;
	}
	else {
		rc = VTM_OK;
		state = 0;
	}

	vtm_socket_set_state_intl(sock, state);
	*out_read = num;

	return rc;
}

static int vtm_socket_plain_dgram_recv(struct vtm_socket *sock, void *buf, size_t maxlen, size_t *out_recv, struct vtm_socket_saddr *saddr)
{
	int rc;
	vtm_sys_sockrc_t cnt;
	struct sockaddr *other;
	vtm_sys_socklen_t *other_len;

#ifdef VTM_SYS_WINDOWS
	if (maxlen > INT_MAX)
		maxlen = INT_MAX;
#endif

	if (saddr) {
		rc = vtm_socket_util_prepare_saddr(sock->family, saddr);
		if (rc != VTM_OK)
			return rc;
		other = (struct sockaddr*) &saddr->addr;
		other_len = &saddr->len;
	}
	else {
		rc = VTM_OK;
		other = NULL;
		other_len = NULL;
	}

	cnt = recvfrom(sock->fd, buf, VTM_SOCKSIZE_CASTED(maxlen), 0, other, other_len);
	if (cnt < 0) {
		rc = vtm_socket_util_read_error(sock);
		cnt = 0;
	}

	*out_recv = cnt;

	return rc;
}

static int vtm_socket_plain_dgram_send(struct vtm_socket *sock, const void *buf, size_t len, size_t *out_send, const struct vtm_socket_saddr *saddr)
{
	int rc;
	vtm_sys_sockrc_t cnt;

#ifdef VTM_SYS_WINDOWS
	if (len > INT_MAX)
		return vtm_err_set(VTM_E_INVALID_ARG);
#endif

	rc = VTM_OK;
	cnt = sendto(sock->fd, buf, VTM_SOCKSIZE_CASTED(len), 0, (struct sockaddr*) &saddr->addr, saddr->len);
	if (cnt < 0) {
		rc = vtm_socket_util_write_error(sock);
		cnt = 0;
	}

	*out_send = cnt;

	return rc;
}
