/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/net/socket.h>

#include <stdlib.h> /* malloc() */
#include <string.h> /* memset() */
#include <netinet/in.h> /* sockaddr_in */
#include <unistd.h> /* close() */

#include <sys/types.h>
#include <sys/socket.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <vtm/core/error.h>
#include <vtm/core/math.h>
#include <vtm/net/common.h>
#include <vtm/net/socket_intl.h>
#include <vtm/sys/base/net/socket_util_intl.h>

#define VTM_SOCKET_TLS_BUF_SIZE     16384

enum vtm_socket_tls_operation
{
	VTM_TLS_OP_ACCEPT,
	VTM_TLS_OP_CONNECT,
	VTM_TLS_OP_READ,
	VTM_TLS_OP_WRITE,
	VTM_TLS_OP_SHUTDOWN
};

struct vtm_socket_tls_info
{
	SSL_CTX *ctx;
	SSL *ssl;
	bool free_ctx;
	bool use_buffers;
	void *recv_buf;
	size_t recv_buf_len;
	size_t recv_buf_used;
	size_t send_want_bytes;
};

/* forward declaration */
static vtm_socket* vtm_socket_tls_alloc(enum vtm_socket_family fam, int sockfd, SSL_CTX *ctx, SSL *ssl, bool free_ctx);

static SSL_CTX* vtm_socket_tls_create_ctx(struct vtm_socket_tls_opts *opts);
static SSL_CTX* vtm_socket_tls_get_ctx(struct vtm_socket *sock);
static SSL* vtm_socket_tls_get_ssl(struct vtm_socket *sock);

static void   vtm_socket_tls_free(struct vtm_socket *sock);
static int    vtm_socket_tls_accept(struct vtm_socket *sock, struct vtm_socket **client);
static int    vtm_socket_tls_connect(struct vtm_socket *sock, const char* host, unsigned int port);
static int    vtm_socket_tls_shutdown(struct vtm_socket *sock, int dir);
static int    vtm_socket_tls_close(struct vtm_socket *sock);
static int    vtm_socket_tls_write(struct vtm_socket *sock, const void *src, size_t len, size_t *out_written);
static int    vtm_socket_tls_read(struct vtm_socket *sock, void *buf, size_t len, size_t *out_read);
static size_t vtm_socket_tls_read_buf(struct vtm_socket_tls_info *info, void *buf, size_t len);
static int    vtm_socket_tls_dgram_recv(struct vtm_socket *sock, void *buf, size_t maxlen, size_t *out_recv, struct vtm_socket_saddr *saddr);
static int    vtm_socket_tls_dgram_send(struct vtm_socket *sock, const void *buf, size_t len, size_t *out_send, const struct vtm_socket_saddr *saddr);
static int    vtm_socket_tls_convert_error(struct vtm_socket *sock, SSL *ssl, int code, enum vtm_socket_tls_operation op);
static int    vtm_socket_tls_save_error(int code);
static int    vtm_socket_tls_set_opt(struct vtm_socket *sock, int opt, const void *val, size_t len);
static int    vtm_socket_tls_enable_buffers(struct vtm_socket *sock);
static void   vtm_socket_tls_disable_buffers(struct vtm_socket *sock);
static void   vtm_socket_tls_release_buffers(struct vtm_socket *sock);

/* vtable */
static struct vtm_socket_vtable vtm_socket_tls_vtable = {
	.vtm_socket_free = vtm_socket_tls_free,
	.vtm_socket_bind = vtm_socket_util_bind,
	.vtm_socket_listen = vtm_socket_util_listen,
	.vtm_socket_accept = vtm_socket_tls_accept,
	.vtm_socket_connect = vtm_socket_tls_connect,
	.vtm_socket_shutdown = vtm_socket_tls_shutdown,
	.vtm_socket_close = vtm_socket_tls_close,
	.vtm_socket_write = vtm_socket_tls_write,
	.vtm_socket_read = vtm_socket_tls_read,
	.vtm_socket_dgram_recv = vtm_socket_tls_dgram_recv,
	.vtm_socket_dgram_send = vtm_socket_tls_dgram_send,
	.vtm_socket_set_opt = vtm_socket_tls_set_opt,
	.vtm_socket_get_opt = vtm_socket_util_get_opt,
	.vtm_socket_get_remote_addr = vtm_socket_util_get_remote_addr
};

vtm_socket* vtm_socket_tls_new(enum vtm_socket_family fam, struct vtm_socket_tls_opts *opts)
{
	int rc;
	int sockfam;
	vtm_sys_socket_t sockfd;
	vtm_socket *sock;
	SSL_CTX *ctx;
	SSL *ssl;

	rc = vtm_socket_util_convert_family(fam, &sockfam);
	if (rc != VTM_OK)
		return NULL;

	sockfd = socket(sockfam, SOCK_STREAM, 0);
	if (VTM_SOCK_INVALID(sockfd)) {
		vtm_socket_util_error(NULL);
		return NULL;
	}

	rc = vtm_socket_util_block_sigpipe(sockfd);
	if (rc != VTM_OK)
		goto err_close;

	ctx = vtm_socket_tls_create_ctx(opts);
	if (!ctx)
		goto err_close;

	ssl = NULL;
	if (!opts->is_server) {
		ssl = SSL_new(ctx);
		if (!ssl) {
			vtm_socket_tls_save_error(VTM_ERROR);
			goto err_ctx;
		}
		SSL_set_fd(ssl, sockfd);
	}

	sock = vtm_socket_tls_alloc(fam, sockfd, ctx, ssl, true);
	if (!sock)
		goto err_ssl;

	return sock;

err_ssl:
	SSL_free(ssl);

err_ctx:
	SSL_CTX_free(ctx);

err_close:
	VTM_CLOSESOCKET(sockfd);

	return NULL;
}

static vtm_socket* vtm_socket_tls_alloc(enum vtm_socket_family fam, int sockfd, SSL_CTX *ctx, SSL *ssl, bool free_ctx)
{
	vtm_socket *sock;
	struct vtm_socket_tls_info *info;

	sock = malloc(sizeof(struct vtm_socket));
	if (!sock) {
		vtm_err_oom();
		return NULL;
	}

	info = malloc(sizeof(struct vtm_socket_tls_info));
	if (!info) {
		vtm_err_oom();
		goto err;
	}

	if (vtm_socket_base_init(sock) != VTM_OK)
		goto err;

	info->ssl = ssl;
	info->ctx = ctx;
	info->free_ctx = free_ctx;
	info->use_buffers = false;
	info->recv_buf = NULL;
	info->recv_buf_len = 0;
	info->recv_buf_used = 0;
	info->send_want_bytes = 0;

	sock->fd = sockfd;
	sock->family = fam;
	sock->type = VTM_SOCK_TYPE_STREAM;
	sock->info = info;
	sock->vtable = &vtm_socket_tls_vtable;

	return sock;

err:
	free(sock);
	return NULL;
}

static void vtm_socket_tls_free(struct vtm_socket *sock)
{
	struct vtm_socket_tls_info *info;

	info = sock->info;
	if (info->ssl)
		SSL_free(info->ssl);
	if (info->free_ctx)
		SSL_CTX_free(info->ctx);
	if (info->use_buffers)
		vtm_socket_tls_release_buffers(sock);

	free(sock->info);
	free(sock);
}

static int vtm_socket_tls_accept(struct vtm_socket *sock, struct vtm_socket **client)
{
	int rc;
	int sockfd;
	struct sockaddr_in client_addr;
	socklen_t length;
	struct vtm_socket *out;
	SSL_CTX *ctx;
	SSL *ssl;

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

	ctx = vtm_socket_tls_get_ctx(sock);
	ssl = SSL_new(ctx);
	if (!ssl) {
		rc = vtm_socket_tls_save_error(VTM_ERROR);
		goto err_sock;
	}

	SSL_set_fd(ssl, sockfd);
	rc = SSL_accept(ssl);
	if (rc == 0) {
		rc = VTM_E_IO_CANCELED;
		goto err_ssl;
	}
	else if (rc < 0) {
		rc = vtm_socket_tls_convert_error(NULL, ssl, rc, VTM_TLS_OP_ACCEPT);
		goto err_ssl;
	}

	out = vtm_socket_tls_alloc(sock->family, sockfd, ctx, ssl, false);
	if (!out) {
		rc = VTM_ERROR;
		goto err_ssl;
	}

	*client = out;
	return VTM_OK;

err_ssl:
	SSL_free(ssl);

err_sock:
	VTM_CLOSESOCKET(sockfd);
	return rc;
}

static int vtm_socket_tls_connect(struct vtm_socket *sock, const char* host, unsigned int port)
{
	int rc;
	SSL *ssl;

	rc = vtm_socket_util_connect(sock, host, port);
	if (rc != VTM_OK)
		return rc;

	ssl = vtm_socket_tls_get_ssl(sock);
	rc = SSL_connect(ssl);
	if (rc <= 0)
		return vtm_socket_tls_convert_error(sock, ssl, rc, VTM_TLS_OP_CONNECT);

	return VTM_OK;
}

static int vtm_socket_tls_shutdown(struct vtm_socket *sock, int dir)
{
	int rc;
	SSL *ssl;

	ssl = vtm_socket_tls_get_ssl(sock);
	if (!ssl)
		return VTM_ERROR;

	rc = SSL_shutdown(ssl);
	if (rc <= 0)
		return vtm_socket_tls_convert_error(sock, ssl, rc, VTM_TLS_OP_SHUTDOWN);

	return VTM_OK;
}

static int vtm_socket_tls_close(struct vtm_socket *sock)
{
	SSL *ssl;

	ssl = vtm_socket_tls_get_ssl(sock);
	if (ssl)
		SSL_shutdown(ssl);

	VTM_CLOSESOCKET(sock->fd);

	return VTM_OK;
}

static int vtm_socket_tls_write(struct vtm_socket *sock, const void *src, size_t len, size_t *out_written)
{
	int rc, num, wlen;
	size_t written;
	struct vtm_socket_tls_info *info;
	SSL *ssl;

	info = sock->info;
	ssl = info->ssl;
	written = 0;

	while (written != len) {
		wlen = VTM_MIN(INT_MAX, len-written);

		/* pending WANT_WRITE repetition? */
		if (info->use_buffers && info->send_want_bytes > 0) {
			wlen = info->send_want_bytes;
			info->send_want_bytes = 0;
		}

		num = SSL_write(ssl, (const char*) src + written, (int) wlen);
		if (num <= 0) {
			rc = vtm_socket_tls_convert_error(sock, ssl, num, VTM_TLS_OP_WRITE);
			if (info->use_buffers && rc == VTM_E_IO_AGAIN)
				info->send_want_bytes = wlen;
			goto out;
		}

		written += (size_t) num;
	}

	rc = VTM_OK;

out:
	*out_written = written;
	return rc;
}

static int vtm_socket_tls_read(struct vtm_socket *sock, void *buf, size_t len, size_t *out_read)
{
	int rc, num;
	struct vtm_socket_tls_info *info;
	SSL *ssl;

	rc = VTM_OK;
	info = sock->info;
	ssl = info->ssl;

	/* OpenSSL expects length as int */
	if (len > INT_MAX)
		len = INT_MAX;

	/* use buffers for non-blocking mode? */
	if (info->use_buffers) {

		/* pending data in buffer available? */
		if (info->recv_buf_used > 0) {
			num = (int) vtm_socket_tls_read_buf(info, buf, len);
			goto out;
		}

		num = SSL_read(ssl, info->recv_buf, (int) info->recv_buf_len);
		if (num <= 0) {
			rc = vtm_socket_tls_convert_error(sock, ssl, num, VTM_TLS_OP_READ);
			num = 0;
		}
		else {
			info->recv_buf_used = (size_t) num;
			num = (int) vtm_socket_tls_read_buf(info, buf, len);
		}
		goto out;
	}

	/* read directly into provided buffer */
	num = SSL_read(ssl, buf, (int) len);
	if (num <= 0) {
		rc = vtm_socket_tls_convert_error(sock, ssl, num, VTM_TLS_OP_READ);
		num = 0;
	}

out:
	*out_read = num;
	return rc;
}

static size_t vtm_socket_tls_read_buf(struct vtm_socket_tls_info *info, void *buf, size_t len)
{
	size_t read;

	read = VTM_MIN(len, info->recv_buf_used);
	memcpy(buf, info->recv_buf, read);
	if (read == info->recv_buf_used) {
		info->recv_buf_used = 0;
	}
	else {
		info->recv_buf_used -= read;
		memmove(info->recv_buf, ((char*) info->recv_buf) + read,
				info->recv_buf_used);
	}

	return read;
}

static int vtm_socket_tls_dgram_recv(struct vtm_socket *sock, void *buf, size_t maxlen, size_t *out_recv, struct vtm_socket_saddr *saddr)
{
	return vtm_err_set(VTM_E_NOT_SUPPORTED);
}

static int vtm_socket_tls_dgram_send(struct vtm_socket *sock, const void *buf, size_t len, size_t *out_send, const struct vtm_socket_saddr *saddr)
{
	return vtm_err_set(VTM_E_NOT_SUPPORTED);
}

static SSL_CTX* vtm_socket_tls_get_ctx(struct vtm_socket *sock)
{
	return ((struct vtm_socket_tls_info*) sock->info)->ctx;
}

static SSL* vtm_socket_tls_get_ssl(struct vtm_socket *sock)
{
	return ((struct vtm_socket_tls_info*) sock->info)->ssl;
}

static SSL_CTX* vtm_socket_tls_create_ctx(struct vtm_socket_tls_opts *opts)
{
	SSL_CTX *ctx;
	const SSL_METHOD *method;
	const char *ciphers;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	method = (opts->is_server) ? TLS_server_method() : TLS_client_method();
	ctx = SSL_CTX_new(method);
	if (!ctx)
		goto err;

	if (SSL_CTX_set_min_proto_version(ctx, TLS1_VERSION) != 1)
		goto err;
#else
	method = (opts->is_server) ? SSLv23_server_method() : SSLv23_client_method();
	ctx = SSL_CTX_new((SSL_METHOD*) method);
	if (!ctx)
		goto err;

	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
#endif

	/* setup certificates */
	if (opts->is_server) {
		if (SSL_CTX_use_certificate_chain_file(ctx, opts->cert_file) != 1)
			goto err;

		if (SSL_CTX_use_PrivateKey_file(ctx, opts->key_file, SSL_FILETYPE_PEM) != 1)
			goto err;
	}
	else if (!opts->no_cert_check) {
		if (SSL_CTX_load_verify_locations(ctx, opts->ca_file, NULL) != 1)
			goto err;

		SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
	}

	/* set cipher list */
	ciphers = opts->ciphers;
	if (!ciphers)
		ciphers = VTM_SOCKET_TLS_DEFAULT_CIPHERS;

	if (SSL_CTX_set_cipher_list(ctx, ciphers) != 1)
		goto err;

	/* set options */
	SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY |
			SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	return ctx;

err:
	vtm_socket_tls_save_error(VTM_ERROR);
	if (ctx)
		SSL_CTX_free(ctx);
	return NULL;
}

static int vtm_socket_tls_convert_error(struct vtm_socket *sock, SSL *ssl, int code, enum vtm_socket_tls_operation op)
{
	int rc;
	unsigned int state;

	state = 0;
	rc = SSL_get_error(ssl, code);
	switch (rc) {
		case SSL_ERROR_NONE:
			switch (op) {
				case VTM_TLS_OP_ACCEPT:
				case VTM_TLS_OP_CONNECT:
				case VTM_TLS_OP_READ:
					state = VTM_SOCK_STAT_READ_AGAIN;
					break;

				case VTM_TLS_OP_WRITE:
					state = VTM_SOCK_STAT_WRITE_AGAIN;
					break;

				default:
					break;
			}
			rc = VTM_E_IO_AGAIN;
			break;

		case SSL_ERROR_WANT_READ:
			switch (op) {
				case VTM_TLS_OP_ACCEPT:
				case VTM_TLS_OP_CONNECT:
				case VTM_TLS_OP_READ:
					state = VTM_SOCK_STAT_READ_AGAIN;
					break;

				case VTM_TLS_OP_WRITE:
					state = VTM_SOCK_STAT_WRITE_AGAIN_WHEN_READABLE;
					break;

				default:
					break;
			}
			rc = VTM_E_IO_AGAIN;
			break;

		case SSL_ERROR_WANT_WRITE:
			switch (op) {
				case VTM_TLS_OP_ACCEPT:
				case VTM_TLS_OP_CONNECT:
				case VTM_TLS_OP_READ:
					state = VTM_SOCK_STAT_READ_AGAIN_WHEN_WRITEABLE;
					break;

				case VTM_TLS_OP_WRITE:
					state = VTM_SOCK_STAT_WRITE_AGAIN;
					break;

				default:
					break;
			}
			rc = VTM_E_IO_AGAIN;
			break;

		case SSL_ERROR_ZERO_RETURN:
			state = VTM_SOCK_STAT_HUP;
			rc = VTM_E_IO_EOF;
			break;

		case SSL_ERROR_SSL:
			state = VTM_SOCK_STAT_ERR;
			rc = VTM_E_IO_PROTOCOL;
			vtm_socket_tls_save_error(rc);
			break;

		default:
			state = VTM_SOCK_STAT_ERR;
			rc = VTM_E_IO_UNKNOWN;
			vtm_socket_tls_save_error(rc);
			break;
	}

	if (sock && state > 0 )
		vtm_socket_set_state_intl(sock, state);

	return rc;
}

static int vtm_socket_tls_save_error(int code)
{
	char buf[256];
	unsigned long num;
	const char *file, *data;
	int line, flags;

	memset(buf, 0, sizeof(buf));
	num = ERR_get_error_line_data(&file, &line, &data, &flags);
	if (num == 0)
		return vtm_err_set(code);

	ERR_error_string_n(num, buf, sizeof(buf));
	return vtm_err_setf(code, "ssl: %s:%s:%d:%s",
		buf, file, line, (flags & ERR_TXT_STRING) ? data : "");
}

static int vtm_socket_tls_set_opt(struct vtm_socket *sock, int opt, const void *val, size_t len)
{
	int rc;

	rc = vtm_socket_util_set_opt(sock, opt, val, len);
	if (rc == VTM_OK) {
		switch (opt) {
			case VTM_SOCK_OPT_NONBLOCKING:
				if (*((bool*)val))
					rc = vtm_socket_tls_enable_buffers(sock);
				else
					vtm_socket_tls_disable_buffers(sock);
				break;
		}
	}
	return rc;
}

static int vtm_socket_tls_enable_buffers(struct vtm_socket *sock)
{
	struct vtm_socket_tls_info *info;

	info = sock->info;

	if (info->use_buffers)
		return VTM_OK;

	info->recv_buf = malloc(VTM_SOCKET_TLS_BUF_SIZE);
	if (!info->recv_buf) {
		vtm_err_oom();
		return vtm_err_get_code();
	}

	info->recv_buf_len = VTM_SOCKET_TLS_BUF_SIZE;
	info->recv_buf_used = 0;
	info->send_want_bytes = 0;
	info->use_buffers = true;

	return VTM_OK;
}

static void vtm_socket_tls_disable_buffers(struct vtm_socket *sock)
{
	struct vtm_socket_tls_info *info;

	info = sock->info;

	if (info->use_buffers) {
		vtm_socket_tls_release_buffers(sock);
		info->use_buffers = false;
	}
}

static void vtm_socket_tls_release_buffers(struct vtm_socket *sock)
{
	struct vtm_socket_tls_info *info;

	info = sock->info;

	free(info->recv_buf);
}
