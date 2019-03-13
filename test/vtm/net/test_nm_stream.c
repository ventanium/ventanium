/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <string.h>
#include <vtm/core/error.h>
#include <vtm/crypto/crypto.h>
#include <vtm/net/nm/nm_stream_client.h>
#include <vtm/net/nm/nm_stream_server.h>
#include <vtm/util/latch.h>
#include <vtm/util/thread.h>

extern void test_nm_prepare_request(vtm_dataset *msg);
extern void test_nm_prepare_response(vtm_dataset *msg);
extern void test_nm_check_response(vtm_dataset *msg);

static vtm_thread *th;
static vtm_nm_stream_srv *srv;
static struct vtm_latch latch;

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

static void server_ready(vtm_nm_stream_srv *srv, struct vtm_nm_stream_srv_opts *opts)
{
	vtm_latch_count(&latch);
}

static void client_msg(vtm_nm_stream_srv *srv, vtm_dataset *wd, vtm_nm_stream_con *con, vtm_dataset *msg)
{
	test_nm_prepare_response(msg);
	vtm_nm_stream_con_send(con, msg);
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

static void test_client(struct vtm_nm_stream_srv_opts *srv_opts)
{
	int rc;
	vtm_nm_stream_client *cl;
	struct vtm_nm_stream_client_opts opts;
	vtm_dataset *msg;

	/* create client */
	cl = vtm_nm_stream_client_new();
	VTM_TEST_ASSERT(cl != NULL, "nm stream client new");

	/* set timeout */
	vtm_nm_stream_client_set_opt(cl, VTM_NM_STREAM_CL_OPT_RECV_TIMEOUT,
		(unsigned long[]) {1000}, sizeof(unsigned long));

	/* connect client */
	opts.addr = srv_opts->addr;
	opts.tls = srv_opts->tls;
	rc = vtm_nm_stream_client_connect(cl, &opts);
	VTM_TEST_ASSERT(rc == VTM_OK, "nm stream connect");

	/* prepare message */
	msg = vtm_dataset_new();
	VTM_TEST_ASSERT(msg != NULL, "nm stream message");
	test_nm_prepare_request(msg);

	/* send message */
	rc = vtm_nm_stream_client_send(cl, msg);
	VTM_TEST_ASSERT(rc == VTM_OK, "nm stream send");

	/* receive response */
	vtm_dataset_clear(msg);
	rc = vtm_nm_stream_client_recv(cl, msg);
	VTM_TEST_CHECK(rc == VTM_OK, "nm stream recv");

	/* check response */
	test_nm_check_response(msg);

	/* free resources */
	vtm_dataset_free(msg);
	vtm_nm_stream_client_free(cl);
	VTM_TEST_PASSED("nm stream client free");
}

static void test_stream_server(void)
{
	struct vtm_nm_stream_srv_opts opts;

	memset(&opts, 0, sizeof(opts));
	opts.addr.family = VTM_SOCK_FAM_IN4;
	opts.addr.host = "127.0.0.1";
	opts.addr.port = 19041;
	opts.tls.enabled = false;
	opts.threads = 0;
	opts.cbs.server_ready = server_ready;
	opts.cbs.client_msg = client_msg;

	/* test plain single-threaded */
	VTM_TEST_LABEL("nm_stream-plain-single");
	start_server(&opts);
	test_client(&opts);
	stop_server();

	/* test plain multi-threaded */
	VTM_TEST_LABEL("nm_stream-plain-multi");
	opts.threads = 4;
	start_server(&opts);
	test_client(&opts);
	stop_server();

#ifdef VTM_MODULE_CRYPTO
	/* test TLS single-threaded */
	VTM_TEST_LABEL("nm_stream-tls-single");
	opts.threads = 0;
	opts.tls.enabled = true;
	opts.tls.cert_file = "./test/data/net/cert.pem";
	opts.tls.key_file = "./test/data/net/key.pem";
	start_server(&opts);
	test_client(&opts);
	stop_server();

	/* test TLS multi-threaded */
	VTM_TEST_LABEL("nm_stream-tls-multi");
	opts.threads = 4;
	start_server(&opts);
	test_client(&opts);
	stop_server();
#endif
}

extern void test_vtm_net_nm_stream(void)
{
	VTM_TEST_LABEL("nm_stream");
	init_modules();
	test_stream_server();
	end_modules();
}
