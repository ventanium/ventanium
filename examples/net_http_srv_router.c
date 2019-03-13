/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <string.h>
#include <vtm/core/error.h>
#include <vtm/fs/mime.h>
#include <vtm/net/http/http_file_route.h>
#include <vtm/net/http/http_server.h>
#include <vtm/net/http/http_router.h>
#include <vtm/net/http/http_util.h>
#include <vtm/util/json.h>
#include <vtm/util/signal.h>

static vtm_http_srv *srv;
static vtm_http_router *rtr;

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

void http_list_values(vtm_http_res *res, vtm_dataset *ds)
{
	vtm_list *entries;
	struct vtm_dataset_entry *entry;
	size_t i, count;

	if (!ds)
		return;

	entries = vtm_dataset_entryset(ds);
	if (!entries)
		return;

	count = vtm_list_size(entries);
	for (i=0; i < count; i++) {
		entry = vtm_list_get_pointer(entries, i);
		vtm_http_res_body_str(res, entry->name);
		vtm_http_res_body_str(res, ": ");
		vtm_http_res_body_str(res, vtm_variant_as_str(entry->var));
		vtm_http_res_body_str(res, "\n");
	}
	vtm_list_free(entries);
}

int http_info(struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res)
{
	vtm_http_res_begin(res, VTM_HTTP_RES_MODE_CHUNKED, VTM_HTTP_200_OK);

	/* path */
	vtm_http_res_body_str(res, "Requested path was: ");
	vtm_http_res_body_str(res, req->path);
	vtm_http_res_body_str(res, "\n");

	/* version */
	vtm_http_res_body_str(res, "HTTP Version: ");
	vtm_http_res_body_str(res, vtm_http_get_version_string(req->version));
	vtm_http_res_body_str(res, "\n");

	/* params */
	if (req->params) {
		vtm_http_res_body_str(res, "--Params--\n");
		http_list_values(res, req->params);
	}

	/* headers */
	if (req->headers) {
		vtm_http_res_body_str(res, "--Headers--\n");
		http_list_values(res, req->headers);
	}

	vtm_http_res_end(res);

	return VTM_OK;
}

int http_auth(struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res)
{
	int rc;
	char *user, *pass;

	rc = vtm_http_basic_auth_read(req, &user, &pass);

	if (rc == VTM_OK && strcmp(user, "test") == 0 && strcmp(pass, "abcd") == 0) {
		vtm_http_res_begin(res, VTM_HTTP_RES_MODE_CHUNKED, VTM_HTTP_200_OK);
		vtm_http_res_set_date(res);
		vtm_http_res_body_str(res, "Access granted!");
		vtm_http_res_end(res);
	}
	else {
		vtm_http_res_begin(res, VTM_HTTP_RES_MODE_FIXED, VTM_HTTP_401_UNAUTHORIZED);
		vtm_http_basic_auth_require(res, "User: test / Password: abcd");
		vtm_http_res_end(res);
	}

	return VTM_OK;
}

int http_json(struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res)
{
	int rc;
	struct vtm_buf buf;

	if (!req->headers)
		return VTM_E_NOT_HANDLED;

	vtm_buf_init(&buf, VTM_BYTEORDER_LE);
	rc = vtm_json_encode_ds(req->headers, &buf);
	if (rc != VTM_OK)
		goto end;

	vtm_buf_putc(&buf, '\0');

	vtm_http_res_begin(res, VTM_HTTP_RES_MODE_FIXED, VTM_HTTP_200_OK);
	vtm_http_res_header(res, VTM_HTTP_HEADER_CONTENT_TYPE, VTM_MIME_APP_JSON);
	vtm_http_res_body_str(res, (char*) buf.data);
	vtm_http_res_end(res);

end:
	vtm_buf_release(&buf);

	return rc;
}

void http_request(struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res)
{
	int rc;

	rc = vtm_http_router_handle(rtr, ctx, req, res);
	if (rc == VTM_OK)
		return;

	vtm_http_res_begin(res, VTM_HTTP_RES_MODE_FIXED, VTM_HTTP_404_NOT_FOUND);
	vtm_http_res_body_str(res, "404 - Not found");
	vtm_http_res_end(res);
}

int main(void)
{
	int rc;
	struct vtm_http_srv_opts opts;
	struct vtm_http_route *file_rt;

	/* register signal handler */
	vtm_signal_set_handler(VTM_SIG_INT, stop_server);

	/* init network module */
	rc = vtm_module_network_init();
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* create router */
	rtr = vtm_http_router_new();
	if (!rtr) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* add routes */
	file_rt = vtm_http_file_rt_new("../");
	if (!file_rt) {
		vtm_err_print();
		goto end;
	}

	vtm_http_router_static_rt(rtr, "/info", http_info);
	vtm_http_router_static_rt(rtr, "/auth", http_auth);
	vtm_http_router_static_rt(rtr, "/json", http_json);
	vtm_http_router_add_rt   (rtr, "/file/", file_rt);

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

	/* free server */
	vtm_http_srv_free(srv);

end:
	/* free router */
	vtm_http_router_free(rtr);

	/* module cleanup */
	vtm_module_network_end();

	return 0;
}
