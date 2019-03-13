/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <string.h>
#include <vtm/core/error.h>
#include <vtm/net/http/http_server.h>
#include <vtm/util/signal.h>

vtm_http_srv *srv;

void stop_server(int psig)
{
	vtm_signal_safe_puts("Stopping server...\n");
	vtm_http_srv_stop(srv);
}

void http_ready(vtm_http_srv *srv, struct vtm_http_srv_opts *opts)
{
	printf("Server URL: http://%s:%u/\n", opts->host, opts->port);
	fflush(stdout);
}

void http_request(struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res)
{
	vtm_http_res_begin(res, VTM_HTTP_RES_MODE_FIXED, VTM_HTTP_200_OK);
	vtm_http_res_body_str(res, "Hello");
	vtm_http_res_end(res);
}

int main(void)
{
	int rc;
	struct vtm_http_srv_opts opts;

	/* register signal handler */
	vtm_signal_set_handler(VTM_SIG_INT, stop_server);

	/* init network module */
	rc = vtm_module_network_init();
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* set options */
	memset(&opts, 0, sizeof(opts));
	opts.host = "127.0.0.1";
	opts.port = 5000;
	opts.backlog = 10;
	opts.events = 16;
	opts.threads = 4;
	opts.cbs.server_ready = http_ready;
	opts.cbs.http_request = http_request;

	/* create http server */
	srv = vtm_http_srv_new();
	if (!srv) {
		vtm_err_print();
		goto end;
	}

	/* run */
	rc = vtm_http_srv_run(srv, &opts);
	if (rc != VTM_OK)
		vtm_err_print();

	/* free http server */
	vtm_http_srv_free(srv);

end:
	/* module cleanup */
	vtm_module_network_end();

	return 0;
}
