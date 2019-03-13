/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <string.h>
#include <vtm/core/error.h>
#include <vtm/core/macros.h>
#include <vtm/net/http/http_server.h>
#include <vtm/net/http/http_util.h>
#include <vtm/util/signal.h>

#define BIND_ADDR    "127.0.0.1"
#define BIND_PORT    5000

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
	/* check if path is websocket url */
	if (strcmp(req->path, "/echo") == 0) {
		if (vtm_http_is_ws_request(req)) {
			vtm_http_upgrade_to_ws(req, res, NULL);
		}
		else {
			vtm_http_res_begin(res, VTM_HTTP_RES_MODE_FIXED, VTM_HTTP_400_BAD_REQUEST);
			vtm_http_res_body_str(res, "No WS Request");
			vtm_http_res_end(res);
		}
		return;
	}

	/* present test page */
	vtm_http_res_begin(res, VTM_HTTP_RES_MODE_FIXED, VTM_HTTP_200_OK);
	vtm_http_res_body_str(res,

	"<!DOCTYPE html>"
	"<html><head><title>VTM WebSocket Test</title></head>"
	"<body>"
	"<button onclick=\"ws_test()\">Send Message</button>"
	"<p>Message: <input id=\"message\" type=\"text\" value=\"Msg\"></p>"
	"<p>Response: <span id=\"response\"></span></p>"
	"<script>function ws_test() {"
	"var message = document.getElementById('message');"
	"var response = document.getElementById('response');"
	"var connection = new WebSocket('ws://"
	BIND_ADDR ":" VTM_TO_STR(BIND_PORT) "/echo');"
	"connection.onopen = function () {connection.send(message.value);};"
	"connection.onerror = function (err) {response.innerHTML = 'Error: ' + err;};"
	"connection.onmessage = function (e) {response.innerHTML = e.data;};"
	"}</script>"
	"</body></html>"

	);
	vtm_http_res_end(res);
}

void ws_connect(struct vtm_http_ctx *ctx, vtm_ws_con *con)
{
	int rc;
	char buf[64];
	unsigned int port;

	rc = vtm_ws_con_get_remote_info(con, buf, sizeof(buf), &port);
	if (rc != VTM_OK)
		return;

	printf("New WebSocket connection from %s:%u, p=%p\n", buf, port, (void*) con);
}

void ws_close(struct vtm_http_ctx *ctx, vtm_ws_con *con)
{
	printf("Closed WebSocket connection p=%p\n", (void*) con);
}

void ws_message(struct vtm_http_ctx *ctx, struct vtm_ws_msg *msg)
{
	switch (msg->type) {
		case VTM_WS_MSG_TEXT:
			printf("WebSocket received TEXT: %.*s\n", (int) msg->len, (char*) msg->data);
			vtm_ws_con_send_msg(msg->con, VTM_WS_MSG_TEXT, "Hello", strlen("Hello"));
			break;

		case VTM_WS_MSG_PING:
			printf("WebSocket received PING: %.*s\n", (int) msg->len, (char*) msg->data);
			break;

		case VTM_WS_MSG_PONG:
			printf("WebSocket received PONG: %.*s\n", (int) msg->len, (char*) msg->data);
			break;

		default:
			break;
	}
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
	opts.host = BIND_ADDR;
	opts.port = BIND_PORT;
	opts.backlog = 10;
	opts.events = 16;
	opts.threads = 4;
	opts.cbs.server_ready = http_ready;
	opts.cbs.http_request = http_request;
	opts.cbs.ws_connect = ws_connect;
	opts.cbs.ws_close = ws_close;
	opts.cbs.ws_message = ws_message;

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
