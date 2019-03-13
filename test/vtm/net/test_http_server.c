/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <stdio.h>
#include <string.h>
#include <vtm/core/error.h>
#include <vtm/core/format.h>
#include <vtm/crypto/crypto.h>
#include <vtm/net/http/http_client.h>
#include <vtm/net/http/http_server.h>
#include <vtm/net/http/http_util.h>
#include <vtm/net/http/http_router.h>
#include <vtm/net/http/http_file_route.h>
#include <vtm/net/http/http_upgrade.h>
#include <vtm/net/http/ws_client.h>
#include <vtm/util/latch.h>
#include <vtm/util/signal.h>
#include <vtm/util/thread.h>

#define TEST_RT_BIG_SIZE   1000000

static vtm_thread *th;
static vtm_http_srv *srv;
static vtm_http_router *rtr;
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

static void http_not_found(vtm_http_res *res)
{
	vtm_http_res_begin(res, VTM_HTTP_RES_MODE_FIXED, VTM_HTTP_404_NOT_FOUND);
	vtm_http_res_body_str(res, "404 - Not found");
	vtm_http_res_end(res);
}

static int test_rt_param(struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res)
{
	unsigned int a, b;
	char buf[64];

	if (!req->params)
		return VTM_E_NOT_HANDLED;

	a = vtm_dataset_get_uint(req->params, "a");
	b = vtm_dataset_get_uint(req->params, "b");

	buf[vtm_fmt_uint(buf, a+b)] = '\0';

	vtm_http_res_begin(res, VTM_HTTP_RES_MODE_CHUNKED, VTM_HTTP_200_OK);
	vtm_http_res_header(res, "Sum", buf);
	vtm_http_res_end(res);

	return VTM_OK;
}

static int test_rt_big(struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res)
{
	int i;

	vtm_http_res_begin(res, VTM_HTTP_RES_MODE_CHUNKED, VTM_HTTP_200_OK);

	for (i=0; i < TEST_RT_BIG_SIZE; i++) {
		vtm_http_res_body_str(res, "Hello\n");
	}

	vtm_http_res_end(res);

	return VTM_OK;
}

static int test_rt_path(struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res)
{
	vtm_http_res_begin(res, VTM_HTTP_RES_MODE_CHUNKED, VTM_HTTP_200_OK);
	vtm_http_res_body_str(res, req->path);
	vtm_http_res_end(res);

	return VTM_OK;
}

static int test_rt_file(struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res)
{
	const char *filename;
	FILE *fp;

	filename = "./test/data/net/http/test.txt";

	fp = fopen(filename, "r");
	if (!fp) {
		http_not_found(res);
		return VTM_OK;
	}

	vtm_http_res_begin(res, VTM_HTTP_RES_MODE_FIXED, VTM_HTTP_200_OK);
	vtm_http_file_serve(res, filename, fp);
	vtm_http_res_end(res);

	return VTM_OK;
}

static int test_rt_ws(struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res)
{
	if (!vtm_http_is_ws_request(req))
		return VTM_E_NOT_HANDLED;

	return vtm_http_upgrade_to_ws(req, res, NULL);
}

static void http_request(struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res)
{
	int rc;

	rc = vtm_http_router_handle(rtr, ctx, req, res);
	if (rc == VTM_OK)
		return;

	http_not_found(res);
}

static void http_ready(vtm_http_srv *srv, struct vtm_http_srv_opts *opts)
{
	vtm_latch_count(&latch);
}

static void ws_msg(struct vtm_http_ctx *ctx, struct vtm_ws_msg *msg)
{
	if (msg->len == 1 && *((char*)msg->data) == 'A')
		vtm_ws_con_send_msg(msg->con, VTM_WS_MSG_TEXT, "B", 1);
}

static int http_server(void *arg)
{
	int rc;
	struct vtm_http_route *file_rt;

	/* router */
	rtr = vtm_http_router_new();
	if (!rtr)
		goto err;

	file_rt = vtm_http_file_rt_new(".");
	if (!file_rt)
		goto err;

	vtm_http_router_add_rt(rtr, "/files/", file_rt);
	vtm_http_router_static_rt(rtr, "/file", test_rt_file);
	vtm_http_router_static_rt(rtr, "/big", test_rt_big);
	vtm_http_router_static_rt(rtr, "/param", test_rt_param);
	vtm_http_router_static_rt(rtr, "/path", test_rt_path);
	vtm_http_router_static_rt(rtr, "/ws", test_rt_ws);

	srv = vtm_http_srv_new();
	if (!srv)
		goto err;

	rc = vtm_http_srv_run(srv, (struct vtm_http_srv_opts*) arg);
	if (rc != VTM_OK)
		goto err;

	vtm_http_router_free(rtr);
	vtm_http_srv_free(srv);

	return VTM_OK;

err:
	vtm_latch_count(&latch);
	return vtm_err_get_code();
}

static void start_server(struct vtm_http_srv_opts *opts)
{
	vtm_latch_init(&latch, 1);
	th = vtm_thread_new(http_server, opts);
	VTM_TEST_ASSERT(th != NULL, "http server thread startet");
	vtm_latch_await(&latch);
}

static void stop_server(void)
{
	int th_rc;

	vtm_http_srv_stop(srv);
	vtm_thread_join(th);

	th_rc = vtm_thread_get_result(th);
	VTM_TEST_CHECK(th_rc == VTM_OK, "http server thread");

	vtm_thread_free(th);
	vtm_latch_release(&latch);
}

static void test_client(struct vtm_http_client_req *req, struct vtm_http_srv_opts *opts)
{
	int rc;
	vtm_http_client *cl;
	struct vtm_http_client_res res;
	char base_url[256];
	char urlbuf[256];
	char portbuf[8];
	unsigned int sum;

	/* prepare base url */
	portbuf[vtm_fmt_uint(portbuf, opts->port)] = '\0';

	if (opts->tls.enabled)
		strcpy(base_url, "https://");
	else
		strcpy(base_url, "http://");
	strcat(base_url, opts->host);
	strcat(base_url, ":");
	strcat(base_url, portbuf);

	/* create client */
	cl = vtm_http_client_new();
	VTM_TEST_ASSERT(cl != NULL, "http client new");

	/* disable tls cert checking */
	if (opts->tls.enabled)
		vtm_http_client_set_opt(cl, VTM_HTTP_CL_OPT_NO_CERT_CHECK, (bool[]) {true}, sizeof(bool));

	vtm_http_client_set_opt(cl, VTM_HTTP_CL_OPT_TIMEOUT,
		                    (unsigned long[]) {1000}, sizeof(unsigned long));

	/* test request path */
	strcpy(urlbuf, base_url);
	strcat(urlbuf, "/path");
	req->url = urlbuf;

	rc = vtm_http_client_request(cl, req, &res);
	VTM_TEST_ASSERT(rc == VTM_OK, "http client req");
	VTM_TEST_CHECK(res.status_code == VTM_HTTP_200_OK, "http path status code");

	rc = strncmp("/path", res.body, (size_t) res.body_len);
	VTM_TEST_CHECK(rc == 0, "http path response");

	vtm_http_client_res_release(&res);

	/* test: request param */
	strcpy(urlbuf, base_url);
	strcat(urlbuf, "/param?a=39&b=12879&f=dddd");
	req->url = urlbuf;

	rc = vtm_http_client_request(cl, req, &res);
	VTM_TEST_ASSERT(rc == VTM_OK, "http client req");
	VTM_TEST_CHECK(res.status_code == VTM_HTTP_200_OK, "http param status code");
	VTM_TEST_ASSERT(res.headers != NULL, "http res headers");

	sum = vtm_dataset_get_uint(res.headers, "Sum");
	VTM_TEST_CHECK(sum == 39 + 12879, "http param response");

	vtm_http_client_res_release(&res);

	vtm_http_client_free(cl);
	VTM_TEST_PASSED("http client free");
}

#ifdef VTM_MODULE_CRYPTO
static void test_ws_client(struct vtm_http_srv_opts *opts)
{
	int rc;
	struct vtm_ws_client *cl;
	char url[256];
	char portbuf[8];
	struct vtm_ws_msg msg;

	portbuf[vtm_fmt_uint(portbuf, opts->port)] = '\0';

	if (opts->tls.enabled)
		strcpy(url, "https://");
	else
		strcpy(url, "http://");
	strcat(url, opts->host);
	strcat(url, ":");
	strcat(url, portbuf);
	strcat(url, "/ws");

	cl = vtm_ws_client_new();
	VTM_TEST_ASSERT(cl != NULL, "ws client new");

	/* disable tls cert checking */
	if (opts->tls.enabled)
		vtm_ws_client_set_opt(cl, VTM_WS_CL_OPT_NO_CERT_CHECK, (bool[]) {true}, sizeof(bool));

	rc = vtm_ws_client_connect(cl, VTM_SOCK_FAM_IN4, url);
	VTM_TEST_ASSERT(rc == VTM_OK, "ws client connect");

	rc = vtm_ws_client_send(cl, VTM_WS_MSG_TEXT, "A", 1);
	VTM_TEST_CHECK(rc == VTM_OK, "ws client send");

	rc = vtm_ws_client_recv(cl, &msg);
	VTM_TEST_CHECK(rc == VTM_OK, "ws client recv");
	VTM_TEST_CHECK(msg.type == VTM_WS_MSG_TEXT, "ws client response type");
	VTM_TEST_CHECK(msg.len == 1, "ws client response length");
	VTM_TEST_CHECK( *((char*)msg.data) == 'B', "ws client response payload");

	vtm_ws_msg_release(&msg);

	rc = vtm_ws_client_close(cl);
	VTM_TEST_CHECK(rc == VTM_OK, "ws client close");

	vtm_ws_client_free(cl);
	VTM_TEST_PASSED("ws client free");
}
#endif

static void test_http_server(void)
{
	struct vtm_http_srv_opts opts;
	struct vtm_http_client_req req;

	/* options */
	memset(&opts, 0, sizeof(opts));
	opts.host = "127.0.0.1";
	opts.port = 19080;
	opts.backlog = 10;
	opts.events = 16;
	opts.threads = 0;
	opts.cbs.server_ready = http_ready;
	opts.cbs.http_request = http_request;
	opts.cbs.ws_message = ws_msg;

	req.method = VTM_HTTP_METHOD_GET;
	req.version = VTM_HTTP_VER_1_1;
	req.fam = VTM_SOCK_FAM_IN4;
	req.headers = NULL;
	req.body = NULL;
	req.body_len = 0;

	/* test single-threaded */
	VTM_TEST_LABEL("http-plain-single");
	start_server(&opts);
	test_client(&req, &opts);
#ifdef VTM_MODULE_CRYPTO
	test_ws_client(&opts);
#endif
	stop_server();

	/* test multi-threaded */
	VTM_TEST_LABEL("http-plain-multi");
	opts.threads = 4;
	start_server(&opts);
	test_client(&req, &opts);
#ifdef VTM_MODULE_CRYPTO
	test_ws_client(&opts);
#endif
	stop_server();

#ifdef VTM_MODULE_CRYPTO
	/* test TLS single-threaded */
	VTM_TEST_LABEL("http-tls-single");
	opts.threads = 0;
	opts.tls.enabled = true;
	opts.tls.cert_file = "./test/data/net/cert.pem";
	opts.tls.key_file = "./test/data/net/key.pem";
	start_server(&opts);
	test_client(&req, &opts);
	test_ws_client(&opts);
	stop_server();

	/* test TLS multi-threaded */
	VTM_TEST_LABEL("http-tls-multi");
	opts.threads = 4;
	start_server(&opts);
	test_client(&req, &opts);
	test_ws_client(&opts);
	stop_server();
#endif
}

extern void test_vtm_net_http_server(void)
{
	VTM_TEST_LABEL("http");
	init_modules();
	test_http_server();
	end_modules();
}
