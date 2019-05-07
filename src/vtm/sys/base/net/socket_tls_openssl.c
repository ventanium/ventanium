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
#include <vtm/net/socket_intl.h>
#include <vtm/sys/base/net/socket_util_intl.h>

#define VTM_TLS_RENOGOTIATION_RETRIES  512

struct vtm_socket_tls_info
{
	SSL_CTX *ctx;
	SSL* ssl;
	bool free_ctx;
};

/* forward declaration */
static vtm_socket* vtm_socket_tls_alloc(enum vtm_socket_family fam, int sockfd, SSL_CTX *ctx, SSL *ssl, bool free_ctx);

static SSL_CTX* vtm_socket_tls_create_ctx(struct vtm_socket_tls_opts *opts);
static SSL_CTX* vtm_socket_tls_get_ctx(struct vtm_socket *sock);
static SSL* vtm_socket_tls_get_ssl(struct vtm_socket *sock);

static void vtm_socket_tls_free(struct vtm_socket *sock);
static int  vtm_socket_tls_accept(struct vtm_socket *sock, struct vtm_socket **client);
static int  vtm_socket_tls_connect(struct vtm_socket *sock, const char* host, unsigned int port);
static int  vtm_socket_tls_shutdown(struct vtm_socket *sock, int dir);
static int  vtm_socket_tls_close(struct vtm_socket *sock);
static int  vtm_socket_tls_write(struct vtm_socket *sock, const void *src, size_t len, size_t *out_written);
static int  vtm_socket_tls_read(struct vtm_socket *sock, void *buf, size_t len, size_t *out_read);
static int  vtm_socket_tls_dgram_recv(struct vtm_socket *sock, void *buf, size_t maxlen, size_t *out_recv, struct vtm_socket_saddr *saddr);
static int  vtm_socket_tls_dgram_send(struct vtm_socket *sock, const void *buf, size_t len, size_t *out_send, const struct vtm_socket_saddr *saddr);
static int  vtm_socket_tls_check_error(struct vtm_socket *sock, SSL *ssl, int code);
static int  vtm_socket_tls_save_error(int code);
static bool vtm_socket_tls_is_renegotiation(struct vtm_socket *sock, int code);

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
	.vtm_socket_set_opt = vtm_socket_util_set_opt,
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
		rc = vtm_socket_tls_check_error(NULL, ssl, rc);
		if (rc == VTM_E_IO_AGAIN)
			vtm_socket_set_state_intl(sock, VTM_SOCK_STAT_READ_AGAIN);
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
	int rc, retries;
	SSL *ssl;

	rc = vtm_socket_util_connect(sock, host, port);
	if (rc != VTM_OK)
		return rc;

	ssl = vtm_socket_tls_get_ssl(sock);
	retries = 0;

retry:
	rc = SSL_connect(ssl);
	if (rc <= 0) {
		rc = vtm_socket_tls_check_error(sock, ssl, rc);
		if (vtm_socket_tls_is_renegotiation(sock, rc)
			&& retries++ < VTM_TLS_RENOGOTIATION_RETRIES)
			goto retry;
		if (rc == VTM_E_IO_AGAIN)
			vtm_socket_set_state_intl(sock, VTM_SOCK_STAT_READ_AGAIN);
		return rc;
	}

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
		return vtm_socket_tls_check_error(sock, ssl, rc);

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
	int rc, num;
	size_t written;
	SSL *ssl;

	rc = VTM_OK;
	ssl = vtm_socket_tls_get_ssl(sock);

	written = 0;
	while (written != len) {
		num = SSL_write(ssl, (const char*) src + written, len - written);
		if (num <= 0) {
			rc = vtm_socket_tls_check_error(sock, ssl, num);
			if (rc == VTM_E_IO_AGAIN)
				vtm_socket_set_state_intl(sock, VTM_SOCK_STAT_WRITE_AGAIN);
			goto out;
		}
		written += num;
	}

out:
	*out_written = written;
	return rc;
}

static int vtm_socket_tls_read(struct vtm_socket *sock, void *buf, size_t len, size_t *out_read)
{
	int rc, num, rlen, retries;
	SSL *ssl;

	ssl = vtm_socket_tls_get_ssl(sock);
	if (!ssl)
		return vtm_err_set(VTM_E_ASSERT_FAILED);

	rlen = (len < INT_MAX) ? (int) len : INT_MAX;
	retries = 0;

retry:
	rc = VTM_OK;
	num = SSL_read(ssl, buf, rlen);
	if (num <= 0) {
		rc = vtm_socket_tls_check_error(sock, ssl, num);
		if (vtm_socket_tls_is_renegotiation(sock, rc)
			&& retries++ < VTM_TLS_RENOGOTIATION_RETRIES)
			goto retry;
		if (rc == VTM_E_IO_AGAIN)
			vtm_socket_set_state_intl(sock, VTM_SOCK_STAT_READ_AGAIN);
		num = 0;
	}

	*out_read = num;
	return rc;
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
#elif OPENSSL_VERSION_NUMBER >= 0x10000000L
	method = (opts->is_server) ? TLSv1_2_server_method() : TLSv1_2_client_method();
	ctx = SSL_CTX_new(method);
#else
	method = (opts->is_server) ? TLSv1_server_method() : TLSv1_client_method();
	ctx = SSL_CTX_new((SSL_METHOD*) method);
#endif

	if (!ctx)
		goto err;

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
	SSL_CTX_set_mode(ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	return ctx;

err:
	vtm_socket_tls_save_error(VTM_ERROR);
	if (ctx)
		SSL_CTX_free(ctx);
	return NULL;
}

static int vtm_socket_tls_check_error(struct vtm_socket *sock, SSL *ssl, int code)
{
	int rc;
	unsigned int state;

	state = 0;
	rc = SSL_get_error(ssl, code);
	switch (rc) {
		case SSL_ERROR_NONE:
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
			rc = VTM_E_IO_AGAIN;
			break;

		case SSL_ERROR_ZERO_RETURN:
			state = VTM_SOCK_STAT_CLOSED;
			rc = VTM_E_IO_CLOSED;
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

static bool vtm_socket_tls_is_renegotiation(struct vtm_socket *sock, int code)
{
	int rc;
	bool nbl;

	if (code != VTM_E_IO_AGAIN)
		return false;

	rc = vtm_socket_get_opt(sock, VTM_SOCK_OPT_NONBLOCKING, &nbl, sizeof(bool));
	if (rc != VTM_OK)
		return false;

	return !nbl;
}
