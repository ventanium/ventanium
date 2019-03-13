/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <string.h>
#include <vtm/core/error.h>
#include <vtm/net/nm/nm_dgram_client.h>
#include <vtm/net/nm/nm_dgram_server.h>
#include <vtm/util/latch.h>
#include <vtm/util/thread.h>

extern void test_nm_prepare_request(vtm_dataset *msg);
extern void test_nm_prepare_response(vtm_dataset *msg);
extern void test_nm_check_response(vtm_dataset *msg);

static vtm_thread *th;
static vtm_nm_dgram_srv *srv;
static struct vtm_latch latch;

static void init_modules(void)
{
	int rc;

	rc = vtm_module_network_init();
	VTM_TEST_ASSERT(rc == VTM_OK, "module network init");
}

static void end_modules(void)
{
	vtm_module_network_end();
}

static void server_ready(vtm_nm_dgram_srv *srv, struct vtm_nm_dgram_srv_opts *opts)
{
	vtm_latch_count(&latch);
}

static void msg_recv(vtm_nm_dgram_srv *srv, vtm_dataset *wd, vtm_dataset *msg, const struct vtm_socket_saddr *saddr)
{
	test_nm_prepare_response(msg);
	vtm_nm_dgram_srv_send(srv, msg, saddr);
}

static int dgram_server(void *arg)
{
	int rc;

	srv = vtm_nm_dgram_srv_new();
	if (!srv) {
		rc = vtm_err_get_code();
		goto end;
	}

	rc = vtm_nm_dgram_srv_run(srv, (struct vtm_nm_dgram_srv_opts*) arg);
	vtm_nm_dgram_srv_free(srv);

end:
	if (rc != VTM_OK)
		vtm_latch_count(&latch);

	return rc;
}

static void start_server(struct vtm_nm_dgram_srv_opts *opts)
{
	vtm_latch_init(&latch, 1);
	th = vtm_thread_new(dgram_server, opts);
	VTM_TEST_ASSERT(th != NULL, "nm dgram server thread startet");
	vtm_latch_await(&latch);
}

static void stop_server(void)
{
	int th_rc;

	vtm_nm_dgram_srv_stop(srv);
	vtm_thread_join(th);

	th_rc = vtm_thread_get_result(th);
	VTM_TEST_CHECK(th_rc == VTM_OK, "nm dgram server thread");

	vtm_thread_free(th);
	vtm_latch_release(&latch);
}

static void test_client(struct vtm_socket_addr *addr)
{
	int rc;
	vtm_nm_dgram_client *cl;
	vtm_dataset *msg;
	struct vtm_socket_saddr saddr;

	/* create client */
	cl = vtm_nm_dgram_client_new(addr->family);
	VTM_TEST_ASSERT(cl != NULL, "nm dgram client new");

	/* set timeout */
	vtm_nm_dgram_client_set_opt(cl, VTM_NM_DGRAM_CL_OPT_RECV_TIMEOUT,
		(unsigned long[]) {1000}, sizeof(unsigned long));

	/* prepare message */
	msg = vtm_dataset_new();
	VTM_TEST_ASSERT(msg != NULL, "nm dgram message");
	test_nm_prepare_request(msg);

	/* send message */
	rc = vtm_socket_os_addr_build(&saddr, addr);
	VTM_TEST_ASSERT(rc == VTM_OK, "nm dgram dst address");

	rc = vtm_nm_dgram_client_send(cl, msg, &saddr);
	VTM_TEST_ASSERT(rc == VTM_OK, "nm dgram send");

	/* receive response */
	vtm_dataset_clear(msg);
	rc = vtm_nm_dgram_client_recv(cl, msg, &saddr);
	VTM_TEST_CHECK(rc == VTM_OK, "nm dgram recv");

	/* check response */
	test_nm_check_response(msg);

	/* free resources */
	vtm_dataset_free(msg);
	vtm_nm_dgram_client_free(cl);
	VTM_TEST_PASSED("nm dgram client free");
}

static void test_dgram_server(void)
{
	struct vtm_nm_dgram_srv_opts opts;

	memset(&opts, 0, sizeof(opts));
	opts.addr.family = VTM_SOCK_FAM_IN4;
	opts.addr.host = "127.0.0.1";
	opts.addr.port = 19040;
	opts.threads = 0;
	opts.cbs.server_ready = server_ready;
	opts.cbs.msg_recv = msg_recv;

	/* test single-threaded */
	VTM_TEST_LABEL("nm_dgram-single");
	start_server(&opts);
	test_client(&opts.addr);
	stop_server();

	/* test multi-threaded */
	VTM_TEST_LABEL("nm_dgram-multi");
	opts.threads = 4;
	start_server(&opts);
	test_client(&opts.addr);
	stop_server();
}

extern void test_vtm_net_nm_dgram(void)
{
	VTM_TEST_LABEL("nm_dgram");
	init_modules();
	test_dgram_server();
	end_modules();
}
