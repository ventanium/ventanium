/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <string.h>
#include <vtm/core/error.h>
#include <vtm/core/format.h>
#include <vtm/crypto/crypto.h>
#include <vtm/net/nm/nm_stream_client.h>
#include <vtm/net/nm/nm_stream_server.h>
#include <vtm/util/latch.h>
#include <vtm/util/mutex.h>
#include <vtm/util/thread.h>

#define CONNECTIONS      16
#define TXT_SIZE         (4 * 1024 * 1024)

static vtm_thread *th;
static vtm_nm_stream_srv *srv;
static struct vtm_latch latch;
static struct vtm_latch con_latch;
static vtm_mutex *mtx;
static vtm_nm_stream_con *cons[CONNECTIONS];

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

static void response_prepare(vtm_dataset *msg)
{
	uint32_t i;
	char buf[VTM_FMT_CHARS_INT32 + 1];
	char *txt;

	vtm_dataset_clear(msg);
	for (i=0; i < 10; i++) {
		buf[vtm_fmt_uint64(buf, i)] = '\0';
		vtm_dataset_set_uint32(msg, buf, i);
	}

	txt = malloc(TXT_SIZE+1);
	VTM_ASSERT(txt != NULL);

	memset(txt, 'A', TXT_SIZE);
	txt[TXT_SIZE] = '\0';

	vtm_dataset_set_string(msg, "TXT", txt);
	free(txt);
}

static int response_check(vtm_dataset *msg)
{
	uint32_t i;
	char buf[VTM_FMT_CHARS_INT32 + 1];
	const char *txt;

	for (i=0; i < 10; i++) {
		buf[vtm_fmt_uint64(buf, i)] = '\0';
		if (vtm_dataset_get_uint32(msg, buf) != i)
			return VTM_ERROR;
	}

	txt = vtm_dataset_get_string(msg, "TXT");
	if (!txt)
		return VTM_ERROR;

	for (i=0; i < TXT_SIZE; i++) {
		if (txt[i] != 'A')
			return VTM_ERROR;
	}

	return VTM_OK;
}

static void server_ready(vtm_nm_stream_srv *srv, struct vtm_nm_stream_srv_opts *opts)
{
	vtm_latch_count(&latch);
}

static void client_connect(vtm_nm_stream_srv *srv, vtm_dataset *wd, vtm_nm_stream_con *con)
{
	int i;

	vtm_mutex_lock(mtx);
	for (i=0; i < CONNECTIONS; i++) {
		if (cons[i] == NULL) {
			cons[i] = con;
			break;
		}
	}
	vtm_mutex_unlock(mtx);

	vtm_latch_count(&con_latch);
}

static void client_disconnect(vtm_nm_stream_srv *srv, vtm_dataset *wd, vtm_nm_stream_con *con)
{
	int i;

	vtm_mutex_lock(mtx);
	for (i=0; i < CONNECTIONS; i++) {
		if (cons[i] == con) {
			cons[i] = NULL;
			break;
		}
	}
	vtm_mutex_unlock(mtx);
}

static void client_msg(vtm_nm_stream_srv *srv, vtm_dataset *wd, vtm_nm_stream_con *con, vtm_dataset *msg)
{
	int i;

	response_prepare(msg);

	vtm_mutex_lock(mtx);
	for (i=0; i < CONNECTIONS; i++) {
		if (cons[i])
			vtm_nm_stream_con_send(cons[i], msg);
	}
	vtm_mutex_unlock(mtx);
}

static int stream_server(void *arg)
{
	int rc;

	srv = vtm_nm_stream_srv_new();
	if (!srv) {
		rc = vtm_err_get_code();
		goto end;
	}

	rc = vtm_nm_stream_srv_run(srv, (struct vtm_nm_stream_srv_opts*) arg);
	vtm_nm_stream_srv_free(srv);

end:
	if (rc != VTM_OK)
		vtm_latch_count(&latch);

	return rc;
}

static void start_server(struct vtm_nm_stream_srv_opts *opts)
{
	vtm_latch_init(&latch, 1);
	th = vtm_thread_new(stream_server, opts);
	VTM_TEST_ASSERT(th != NULL, "nm stream server thread startet");
	vtm_latch_await(&latch);
	VTM_TEST_ASSERT(vtm_thread_running(th) == true, "thread running");
}

static void stop_server(void)
{
	int th_rc;

	vtm_nm_stream_srv_stop(srv);
	vtm_thread_join(th);

	th_rc = vtm_thread_get_result(th);
	VTM_TEST_CHECK(th_rc == VTM_OK, "nm stream server thread");

	vtm_thread_free(th);
	vtm_latch_release(&latch);
}

static int stream_client(void *arg)
{
	int rc, i;
	vtm_nm_stream_client *cl;
	struct vtm_nm_stream_srv_opts *srv_opts;
	struct vtm_nm_stream_client_opts opts;
	vtm_dataset *msg;

	srv_opts = arg;
	msg = NULL;

	/* create client */
	cl = vtm_nm_stream_client_new();
	if (!cl)
		VTM_TEST_ABORT("Could not create client");

	/* set timeout */
	vtm_nm_stream_client_set_opt(cl, VTM_NM_STREAM_CL_OPT_RECV_TIMEOUT,
		(unsigned long[]) {30000}, sizeof(unsigned long));

	/* connect client */
	opts.addr = srv_opts->addr;
	opts.tls = srv_opts->tls;
	rc = vtm_nm_stream_client_connect(cl, &opts);
	if (rc != VTM_OK)
		VTM_TEST_ABORT("Not connected");

	/* wait for all clients to connect */
	vtm_latch_await(&con_latch);

	/* prepare message */
	msg = vtm_dataset_new();
	if (!msg)
		VTM_TEST_ABORT("Not dataset");
	vtm_dataset_set_uint32(msg, "A", 1);

	/* send message */
	rc = vtm_nm_stream_client_send(cl, msg);
	if (rc != VTM_OK)
		VTM_TEST_ABORT("Could not send request");

	/* receive responses */
	for (i=0; i < CONNECTIONS; i++) {
		vtm_dataset_clear(msg);
retry:
		rc = vtm_nm_stream_client_recv(cl, msg);
		if (rc == VTM_E_INTERRUPTED)
			goto retry;
		if (rc != VTM_OK)
			VTM_TEST_ABORT("No response");

		rc = response_check(msg);
		if (rc != VTM_OK)
			VTM_TEST_ABORT("Wrong response");
	}

	/* free resources */
	vtm_nm_stream_client_close(cl);
	vtm_dataset_free(msg);
	vtm_nm_stream_client_free(cl);

	return VTM_OK;
}

static void test_clients(struct vtm_nm_stream_srv_opts *opts)
{
	int i;
	vtm_thread *ths[CONNECTIONS];

	memset(cons, 0, sizeof(cons));
	vtm_latch_init(&con_latch, CONNECTIONS);

	for (i=0; i < CONNECTIONS; i++) {
		ths[i] = vtm_thread_new(stream_client, opts);
		VTM_TEST_ASSERT(ths[i] != NULL, "client thread creation");
	}

	for (i=0; i < CONNECTIONS; i++) {
		vtm_thread_join(ths[i]);
		VTM_TEST_ASSERT(vtm_thread_get_result(ths[i]) == VTM_OK, "thread result");
		vtm_thread_free(ths[i]);
	}

	vtm_latch_release(&con_latch);
}

static void test_stream_server(void)
{
	struct vtm_nm_stream_srv_opts opts;

	mtx = vtm_mutex_new();
	VTM_TEST_ASSERT(mtx != NULL, "mutex creation");

	memset(&opts, 0, sizeof(opts));
	opts.addr.family = VTM_SOCK_FAM_IN4;
	opts.addr.host = "127.0.0.1";
	opts.addr.port = 19042;
	opts.tls.enabled = false;
	opts.threads = 0;
	opts.cbs.server_ready = server_ready;
	opts.cbs.client_connect = client_connect;
	opts.cbs.client_disconnect = client_disconnect;
	opts.cbs.client_msg = client_msg;

	/* test plain single-threaded */
	VTM_TEST_LABEL("nm_stream_mt-plain-single");
	start_server(&opts);
	test_clients(&opts);
	stop_server();

	/* test plain multi-threaded */
	VTM_TEST_LABEL("nm_stream_mt-plain-multi");
	opts.threads = 4;
	start_server(&opts);
	test_clients(&opts);
	stop_server();

#ifdef VTM_MODULE_CRYPTO
	/* test TLS single-threaded */
	VTM_TEST_LABEL("nm_stream_mt-tls-single");
	opts.threads = 0;
	opts.tls.enabled = true;
	opts.tls.cert_file = "./test/data/net/cert.pem";
	opts.tls.key_file = "./test/data/net/key.pem";
	start_server(&opts);
	test_clients(&opts);
	stop_server();

	/* test TLS multi-threaded */
	VTM_TEST_LABEL("nm_stream_mt-tls-multi");
	opts.threads = 4;
	start_server(&opts);
	test_clients(&opts);
	stop_server();
#endif

	/* cleanup */
	vtm_mutex_free(mtx);
}

extern void test_vtm_net_nm_stream_mt(void)
{
	VTM_TEST_LABEL("nm_stream_mt");
	init_modules();
	test_stream_server();
	end_modules();
}
