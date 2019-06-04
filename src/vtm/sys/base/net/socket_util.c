/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "socket_util_intl.h"

#include <string.h> /* memset() */
#include <errno.h> /* errno */

#ifdef VTM_HAVE_POSIX

	#include <fcntl.h> /* fcntl() */
	#include <arpa/inet.h>
	#include <netinet/in.h> /* sockaddr_in */
	#include <netinet/tcp.h> /* IPPROTO_TCP, TCP_NODELAY */
	#include <sys/types.h>
	#include <sys/time.h>
	#include <sys/socket.h>
	#include <netdb.h>

	#define VTM_SETSOCKOPT_CAST
	#define VTM_GETSOCKOPT_CAST

	#define VTM_SOCK_ERR_AGAIN        EAGAIN
	#define VTM_SOCK_ERR_WOULDBLOCK   EWOULDBLOCK
	#define VTM_SOCK_ERR_CONNABORTED  ECONNABORTED
	#define VTM_SOCK_ERR_INTR         EINTR
	#define VTM_SOCK_ERR_MFILE        EMFILE
	#define VTM_SOCK_ERR_NFILE        ENFILE
	#define VTM_SOCK_ERR_PROTO        EPROTO
	#define VTM_SOCK_ERR_NOBUF        ENOBUFS
	#define VTM_SOCK_ERR_NOMEM        ENOMEM
	#define VTM_SOCK_ERR_PERM         EPERM

#elif VTM_SYS_WINDOWS

	#include <winsock2.h>
	#include <ws2tcpip.h>

	#define VTM_SETSOCKOPT_CAST       (const char*)
	#define VTM_GETSOCKOPT_CAST       (char*)

	#define VTM_SOCK_ERR_AGAIN        WSAEWOULDBLOCK
	#define VTM_SOCK_ERR_WOULDBLOCK   WSAEWOULDBLOCK
	#define VTM_SOCK_ERR_CONNABORTED  WSAECONNRESET
	#define VTM_SOCK_ERR_INTR         WSAEINTR
	#define VTM_SOCK_ERR_MFILE        WSAEMFILE
	#define VTM_SOCK_ERR_NOBUF        WSAENOBUFS

	#ifdef __MINGW32__
	extern int WSAAPI inet_pton(int, const char*, void*);
	#endif

#endif

#include <vtm/core/error.h>
#include <vtm/core/flag.h>
#include <vtm/core/format.h>
#include <vtm/core/lang.h>
#include <vtm/net/socket_addr_intl.h>
#include <vtm/util/signal.h>

#define VTM_SOCK_ERR(rc)              (rc != 0)

static VTM_THREAD_LOCAL int vtm_sig_blocked = 0;

/* forward declaration */
static int vtm_socket_util_bind_ip4(struct vtm_socket *sock, int sockfam, int addr_type, const char *addr, unsigned int port);
static int vtm_socket_util_bind_ip6(struct vtm_socket *sock, int sockfam, int addr_type, const char *addr, unsigned int port);
static int vtm_socket_util_set_tcp_nodelay(struct vtm_socket *sock, bool enabled);
static int vtm_socket_util_get_tcp_nodelay(struct vtm_socket *sock, bool *enabled);
static int vtm_socket_util_set_send_timeout(struct vtm_socket *sock, unsigned long millis);

int vtm_socket_util_block_sigpipe(vtm_sys_socket_t fd)
{
	if (vtm_sig_blocked == 0) {
		vtm_sig_blocked++;
		vtm_signal_block(VTM_SIG_PIPE);
	}

	return VTM_OK;
}

int vtm_socket_util_convert_family(enum vtm_socket_family fam, int *out_fam)
{
	int rc;

	rc = VTM_OK;

	switch (fam) {
		case VTM_SOCK_FAM_IN4:
			*out_fam = AF_INET;
			break;

		case VTM_SOCK_FAM_IN6:
			*out_fam = AF_INET6;
			break;

		default:
			rc = VTM_E_NOT_SUPPORTED;
			break;
	}

	return rc;
}

int vtm_socket_util_convert_type(int type, int *out_type)
{
	int rc;

	rc = VTM_OK;

	switch (type) {
		case VTM_SOCK_TYPE_STREAM:
			*out_type = SOCK_STREAM;
			break;

		case VTM_SOCK_TYPE_DGRAM:
			*out_type = SOCK_DGRAM;
			break;

		default:
			rc = VTM_E_NOT_SUPPORTED;
			break;
	}

	return rc;
}

int vtm_socket_util_prepare_saddr(enum vtm_socket_family fam, struct vtm_socket_saddr *saddr)
{
	switch (fam) {
		case VTM_SOCK_FAM_IN4:
			saddr->len = sizeof(saddr->addr.in4);
			break;

		case VTM_SOCK_FAM_IN6:
			saddr->len = sizeof(saddr->addr.in6);
			break;
	}

	return VTM_OK;
}

int vtm_socket_util_bind(struct vtm_socket *sock, const char *addr, unsigned int port)
{
	int rc;
	int sockfam;
	struct vtm_socket_addr_info addr_info;

	rc = vtm_socket_util_convert_family(sock->family, &sockfam);
	if (rc != VTM_OK)
		return rc;

	addr_info.sock_family = sock->family;
	rc = vtm_socket_addr_get_info(addr, &addr_info);
	if (rc != VTM_OK)
		return rc;

	if (addr_info.addr_type == VTM_SOCK_ADDR_TYPE_INVALID)
		return vtm_err_set(VTM_ERROR);

	setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, VTM_SETSOCKOPT_CAST (int[]) {1}, sizeof(int));

	switch (sock->family) {
		case VTM_SOCK_FAM_IN4:
			rc = vtm_socket_util_bind_ip4(sock, sockfam, addr_info.addr_type, addr, port);
			break;

		case VTM_SOCK_FAM_IN6:
			rc = vtm_socket_util_bind_ip6(sock, sockfam, addr_info.addr_type, addr, port);
			break;

		default:
			rc = VTM_E_NOT_SUPPORTED;
			break;
	}

	return rc;
}

static int vtm_socket_util_bind_ip4(struct vtm_socket *sock, int sockfam, int addr_type, const char *addr, unsigned int port)
{
	int rc;
	struct sockaddr_in bind_addr;

	memset(&bind_addr, 0, sizeof(bind_addr));
	bind_addr.sin_family = sockfam;
	bind_addr.sin_port = htons(port);

	switch (addr_type) {
		case VTM_SOCK_ADDR_TYPE_IPV4_LOOPBACK:
			bind_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			break;

		case VTM_SOCK_ADDR_TYPE_IPV4_ANY:
			bind_addr.sin_addr.s_addr = INADDR_ANY;
			break;

		case VTM_SOCK_ADDR_TYPE_IPV4_SPEC:
			bind_addr.sin_addr.s_addr = inet_addr(addr);
			break;

		default:
			return VTM_E_NOT_SUPPORTED;
	}

	rc = bind(sock->fd, (struct sockaddr*) &bind_addr, sizeof(bind_addr));
	if (VTM_SOCK_ERR(rc))
		return vtm_socket_util_error(sock);

	return VTM_OK;
}

static int vtm_socket_util_bind_ip6(struct vtm_socket *sock, int sockfam, int addr_type, const char *addr, unsigned int port)
{
	int rc;
	struct sockaddr_in6 bind_addr;

	memset(&bind_addr, 0, sizeof(bind_addr));
	bind_addr.sin6_family = sockfam;
	bind_addr.sin6_port = htons(port);

	switch (addr_type) {
		case VTM_SOCK_ADDR_TYPE_IPV6_LOOPBACK:
			bind_addr.sin6_addr = in6addr_loopback;
			break;

		case VTM_SOCK_ADDR_TYPE_IPV6_ANY:
			bind_addr.sin6_addr = in6addr_any;
			break;

		case VTM_SOCK_ADDR_TYPE_IPV6_SPEC:
			rc = inet_pton(sockfam, addr, &(bind_addr.sin6_addr));
			if (rc != 1)
				return vtm_err_set(VTM_ERROR);
			break;

		default:
			return VTM_E_NOT_SUPPORTED;
	}

	rc = bind(sock->fd, (struct sockaddr*) &bind_addr, sizeof(bind_addr));
	if (VTM_SOCK_ERR(rc))
		return vtm_socket_util_error(sock);

	return VTM_OK;
}

int vtm_socket_util_listen(struct vtm_socket *sock, unsigned int backlog)
{
	int rc;

	rc = listen(sock->fd, backlog);
	if (VTM_SOCK_ERR(rc))
		return vtm_socket_util_error(sock);

	return VTM_OK;
}

int vtm_socket_util_connect(struct vtm_socket *sock, const char *host, unsigned int port)
{
	int rc;
	struct vtm_socket_addr addr;
	struct vtm_socket_saddr saddr;

	addr.family = sock->family;
	addr.host = host;
	addr.port = port;

	rc = vtm_socket_os_addr_build(&saddr, &addr);
	if (rc != VTM_OK)
		return rc;

	rc = connect(sock->fd, (struct sockaddr*) &saddr.addr, saddr.len);
	if (VTM_SOCK_ERR(rc))
		return vtm_socket_util_read_error(sock);

	return VTM_OK;
}

int vtm_socket_util_get_remote_addr(struct vtm_socket *sock, struct vtm_socket_saddr *saddr)
{
	int rc;

	vtm_socket_util_prepare_saddr(VTM_SOCK_FAM_IN6, saddr);
	rc = getpeername(sock->fd, &saddr->addr.sa, &saddr->len);
	if (rc != 0)
		return vtm_socket_util_error(sock);

	return VTM_OK;
}

int vtm_socket_util_set_opt(struct vtm_socket *sock, int opt, const void *val, size_t len)
{
	switch (opt) {
		case VTM_SOCK_OPT_NONBLOCKING:
			if (len != sizeof(bool))
				return VTM_E_INVALID_ARG;
			return vtm_socket_util_set_nonblocking(sock->fd, *((bool*)val));

		case VTM_SOCK_OPT_TCP_NODELAY:
			if (len != sizeof(bool))
				return VTM_E_INVALID_ARG;
			return vtm_socket_util_set_tcp_nodelay(sock, *((bool*)val));

		case VTM_SOCK_OPT_RECV_TIMEOUT:
			if (len != sizeof(unsigned long))
				return VTM_E_INVALID_ARG;
			return vtm_socket_util_set_recv_timeout(sock->fd, sock, *((unsigned long*)val));

		case VTM_SOCK_OPT_SEND_TIMEOUT:
			if (len != sizeof(unsigned long))
				return VTM_E_INVALID_ARG;
			return vtm_socket_util_set_send_timeout(sock, *((unsigned long*)val));
	}

	return VTM_E_NOT_SUPPORTED;
}

int vtm_socket_util_get_opt(struct vtm_socket *sock, int opt, void *val, size_t len)
{
	switch (opt) {
		case VTM_SOCK_OPT_TCP_NODELAY:
			if (len != sizeof(bool))
				return VTM_E_INVALID_ARG;
			return vtm_socket_util_get_tcp_nodelay(sock, (bool*) val);

		default:
			break;
	}

	return VTM_E_NOT_SUPPORTED;
}

int vtm_socket_util_error(struct vtm_socket *sock)
{
	int rc, state, err;

#ifdef VTM_SYS_WINDOWS
	err = WSAGetLastError();
#else
	err = errno;
#endif

/* on POSIX EAGAIN und EWOULDBLOCK can be different constants */
#if VTM_SOCK_ERR_AGAIN != VTM_SOCK_ERR_WOULDBLOCK
	if (err == VTM_SOCK_ERR_WOULDBLOCK)
		err = VTM_SOCK_ERR_AGAIN;
#endif

	state = 0;

	switch (err) {
		case VTM_SOCK_ERR_AGAIN:
			rc = VTM_E_IO_AGAIN;
			break;

		case VTM_SOCK_ERR_CONNABORTED:
			rc = VTM_E_IO_CANCELED;
			break;

		case VTM_SOCK_ERR_INTR:
			rc = VTM_E_INTERRUPTED;
			break;

#ifdef VTM_SOCK_ERR_NFILE
		case VTM_SOCK_ERR_NFILE:
#endif
		case VTM_SOCK_ERR_MFILE:
			rc = VTM_E_MAX_REACHED;
			break;

#ifdef VTM_SOCK_ERR_NOMEM
		case VTM_SOCK_ERR_NOMEM:
#endif
		case VTM_SOCK_ERR_NOBUF:
			rc = VTM_E_MEMORY;
			break;

#ifdef VTM_SOCK_ERR_PROTO
		case VTM_SOCK_ERR_PROTO:
			rc = VTM_E_IO_PROTOCOL;
			state = VTM_SOCK_STAT_ERR;
			break;
#endif

#ifdef VTM_SOCK_ERR_PERM
		case VTM_SOCK_ERR_PERM:
			rc = VTM_E_PERMISSION;
			break;
#endif

		default:
			rc = VTM_E_IO_UNKNOWN;
			state = VTM_SOCK_STAT_ERR;
			break;
	}

	vtm_err_setf(rc, "%d: %s", errno, strerror(errno));

	if (sock && state > 0)
		vtm_socket_set_state_intl(sock, state);

	return rc;
}

int vtm_socket_util_read_error(struct vtm_socket *sock)
{
	int rc;

	rc = vtm_socket_util_error(sock);

	if (rc == VTM_E_IO_AGAIN && sock)
		vtm_socket_set_state_intl(sock, VTM_SOCK_STAT_READ_AGAIN);

	return rc;
}

int vtm_socket_util_write_error(struct vtm_socket *sock)
{
	int rc;

	rc = vtm_socket_util_error(sock);

	if (rc == VTM_E_IO_AGAIN && sock)
		vtm_socket_set_state_intl(sock, VTM_SOCK_STAT_WRITE_AGAIN);

	return rc;
}

static int vtm_socket_util_set_tcp_nodelay(struct vtm_socket *sock, bool enabled)
{
	int rc;
	int opt;

	opt = enabled ? 1 : 0;

	rc = setsockopt(sock->fd, IPPROTO_TCP, TCP_NODELAY, VTM_SETSOCKOPT_CAST &opt, sizeof(opt));
	if (rc != 0)
		return vtm_socket_util_error(sock);

	return VTM_OK;
}

static int vtm_socket_util_get_tcp_nodelay(struct vtm_socket *sock, bool *enabled)
{
	int rc;
	int opt;
	socklen_t opt_len;

	opt = 0;
	opt_len = 0;

	rc = getsockopt(sock->fd, IPPROTO_TCP, TCP_NODELAY, VTM_GETSOCKOPT_CAST &opt, &opt_len);
	if (rc != 0)
		return vtm_socket_util_error(sock);

	*enabled = opt ? true : false;

	return VTM_OK;
}

int vtm_socket_util_set_recv_timeout(vtm_sys_socket_t fd, struct vtm_socket *sock, unsigned long millis)
{
	int rc;
#ifdef VTM_HAVE_POSIX
	struct timeval val;
	val.tv_sec = millis / 1000;
	val.tv_usec = (millis % 1000) * 1000;
#elif VTM_SYS_WINDOWS
	DWORD val;
	val = millis;
#endif

	rc = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, VTM_SETSOCKOPT_CAST &val, sizeof(val));
	if (rc != 0)
		return vtm_socket_util_error(sock);

	return VTM_OK;
}

static int vtm_socket_util_set_send_timeout(struct vtm_socket *sock, unsigned long millis)
{
	int rc;
#ifdef VTM_HAVE_POSIX
	struct timeval val;
	val.tv_sec = millis / 1000;
	val.tv_usec = (millis % 1000) * 1000;
#elif VTM_SYS_WINDOWS
	DWORD val;
	val = millis;
#endif

	rc = setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, VTM_SETSOCKOPT_CAST &val, sizeof(val));
	if (rc != 0)
		return vtm_socket_util_error(sock);

	return VTM_OK;
}

#ifdef VTM_HAVE_POSIX

int vtm_socket_util_set_nonblocking(vtm_sys_socket_t fd, bool enabled)
{
	int flags;
	int rc;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
		return VTM_ERROR;

	if (enabled)
		vtm_flag_set(flags, O_NONBLOCK);
	else
		vtm_flag_unset(flags, O_NONBLOCK);

	rc = fcntl(fd, F_SETFL, flags);
	if (rc < 0)
		return VTM_ERROR;

	return VTM_OK;
}

#elif VTM_SYS_WINDOWS

int vtm_socket_util_set_nonblocking(vtm_sys_socket_t fd, bool enabled)
{
	int rc;
	unsigned long mode;

	mode = enabled ? 1 : 0;

	rc = ioctlsocket(fd, FIONBIO, &mode);
	if (rc != 0)
		return VTM_ERROR;

	return VTM_OK;
}

#endif
