/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <string.h>
#include <vtm/core/error.h>
#include <vtm/crypto/crypto.h>
#include <vtm/net/socket.h>
#include <vtm/util/latch.h>
#include <vtm/util/thread.h>

#define BIND_ADDR    "127.0.0.1"
#define BIND_PORT    19070

static vtm_thread *th;
static struct vtm_latch latch;

static int run_endpoint_tcp(vtm_socket *sock);
static void test_tcp_socket_client(vtm_socket *sock);

static void init_modules(void)
{
	int rc;

	rc = vtm_module_crypto_init();
	VTM_TEST_ASSERT(rc == VTM_OK, "module crypto init");

	rc = vtm_module_network_init();
	VTM_TEST_ASSERT(rc == VTM_OK, "module network init");
}

static void end_modules(void)
{
	vtm_module_network_end();
	vtm_module_crypto_end();
}

static void start_endpoint(vtm_thread_func func)
{
	vtm_latch_init(&latch, 1);
	th = vtm_thread_new(func, NULL);
	VTM_TEST_ASSERT(th != NULL, "endpoint thread startet");
	vtm_latch_await(&latch);
	VTM_TEST_ASSERT(vtm_thread_running(th) == true, "thread running");
}

static void clean_endpoint(void)
{
	int th_rc;

	vtm_thread_join(th);

	th_rc = vtm_thread_get_result(th);
	VTM_TEST_CHECK(th_rc == VTM_OK, "endpoint thread result");

	vtm_thread_free(th);
	vtm_latch_release(&latch);
}

static int endpoint_tcp_plain(void *arg)
{
	vtm_socket *sock;

	sock = vtm_socket_new(VTM_SOCK_FAM_IN4, VTM_SOCK_TYPE_STREAM);
	if (!sock)
		return VTM_ERROR;

	return run_endpoint_tcp(sock);
}

#ifdef VTM_MODULE_CRYPTO
static int endpoint_tcp_tls(void *arg)
{
	vtm_socket *sock;
	struct vtm_socket_tls_opts opts;

	memset(&opts, 0, sizeof(opts));
	opts.is_server = true;
	opts.no_cert_check = true;
	opts.cert_file = "./test/data/net/cert.pem";
	opts.key_file = "./test/data/net/key.pem";

	sock = vtm_socket_tls_new(VTM_SOCK_FAM_IN4, &opts);
	if (!sock)
		return VTM_ERROR;

	return run_endpoint_tcp(sock);
}
#endif

static int run_endpoint_tcp(vtm_socket *sock)
{
	int rc;
	vtm_socket *client;
	char buf[64];
	bool latched;
	size_t bytes_read, bytes_written;

	latched = false;

	rc = vtm_socket_bind(sock, BIND_ADDR, BIND_PORT);
	if (rc != VTM_OK)
		goto clean;

	rc = vtm_socket_listen(sock, 5);
	if (rc != VTM_OK)
		goto clean;

	vtm_latch_count(&latch);
	latched = true;

	rc = vtm_socket_accept(sock, &client);
	if (rc != VTM_OK)
		goto clean;

	rc = vtm_socket_read(client, &buf, sizeof(buf), &bytes_read);
	if (rc != VTM_OK)
		goto clean_client;

	rc = vtm_socket_write(client, &buf, bytes_read, &bytes_written);

clean_client:
	vtm_socket_close(client);
	vtm_socket_free(client);

clean:
	vtm_socket_close(sock);
	vtm_socket_free(sock);

	if (!latched)
		vtm_latch_count(&latch);

	return rc;
}

static void test_tcp_socket_plain()
{
	vtm_socket *sock;

	sock = vtm_socket_new(VTM_SOCK_FAM_IN4, VTM_SOCK_TYPE_STREAM);
	VTM_TEST_ASSERT(sock != NULL, "socket plain creation");

	test_tcp_socket_client(sock);
}

#ifdef VTM_MODULE_CRYPTO
static void test_tcp_socket_tls()
{
	vtm_socket *sock;
	struct vtm_socket_tls_opts opts;

	memset(&opts, 0, sizeof(opts));
	opts.is_server = false;
	opts.no_cert_check = true;
	opts.cert_file = "./test/data/net/cert.pem";

	sock = vtm_socket_tls_new(VTM_SOCK_FAM_IN4, &opts);
	VTM_TEST_ASSERT(sock != NULL, "socket tls creation");

	test_tcp_socket_client(sock);
}
#endif

static void test_tcp_socket_client(vtm_socket *sock)
{
	int rc;
	char buf[64];
	size_t bytes_read, bytes_written;

	/* test READ on not connected socket */
	rc = vtm_socket_read(sock, &buf, sizeof(buf), &bytes_read);
	VTM_TEST_CHECK(rc != VTM_OK, "read not connected");
	VTM_TEST_CHECK(bytes_read == 0, "bytes_read not connected");

	/* test WRITE on not connected socket */
	rc = vtm_socket_write(sock, "TEST", strlen("TEST"), &bytes_written);
	VTM_TEST_CHECK(rc != VTM_OK, "write not connected");
	VTM_TEST_CHECK(bytes_written == 0, "bytes_written not connected");

	/* connect */
	rc = vtm_socket_connect(sock, BIND_ADDR, BIND_PORT);
	VTM_TEST_CHECK(rc == VTM_OK, "socket connect");

	/* write connected */
	rc = vtm_socket_write(sock, "TEST", strlen("TEST"), &bytes_written);
	VTM_TEST_CHECK(rc == VTM_OK, "write not connected");
	VTM_TEST_CHECK(bytes_written == strlen("TEST"), "bytes_written connected");

	/* read connected */
	rc = vtm_socket_read(sock, &buf, sizeof(buf), &bytes_read);
	VTM_TEST_CHECK(rc == VTM_OK, "read connected");
	VTM_TEST_CHECK(bytes_read == strlen("TEST"), "bytes_read connected");

	/* close */
	vtm_socket_close(sock);

	/* test READ on closed socket */
	rc = vtm_socket_read(sock, &buf, sizeof(buf), &bytes_read);
	VTM_TEST_CHECK(rc != VTM_OK, "read closed");
	VTM_TEST_CHECK(bytes_read == 0, "bytes_read closed");

	/* test WRITE on closed socket */
	rc = vtm_socket_write(sock, "TEST", strlen("TEST"), &bytes_written);
	VTM_TEST_CHECK(rc != VTM_OK, "write closed");
	VTM_TEST_CHECK(bytes_written == 0, "bytes_written closed");

	vtm_socket_free(sock);
}

static void test_tcp_socket(void)
{
	/* test plain */
	VTM_TEST_LABEL("socket-tcp-plain");
	start_endpoint(endpoint_tcp_plain);
	test_tcp_socket_plain();
	clean_endpoint();

#ifdef VTM_MODULE_CRYPTO
	/* test TLS */
	VTM_TEST_LABEL("socket-tcp-tls");
	start_endpoint(endpoint_tcp_tls);
	test_tcp_socket_tls();
	clean_endpoint();
#endif
}

extern void test_vtm_net_socket(void)
{
	VTM_TEST_LABEL("socket");
	init_modules();
	test_tcp_socket();
	end_modules();
}
