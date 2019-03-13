/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "http_client.h"

#include <string.h> /* memset(), strcmp() */
#include <vtm/core/buffer.h>
#include <vtm/core/error.h>
#include <vtm/core/format.h>
#include <vtm/core/string.h>
#include <vtm/core/types.h>
#include <vtm/net/socket.h>
#include <vtm/net/socket_writer.h>
#include <vtm/net/url.h>
#include <vtm/net/http/http_parser.h>

#define VTM_HTTP_CL_HINT_NO_CERT_CHECK      1

struct vtm_http_client
{
	vtm_socket                 *sock;
	struct vtm_socket_writer   writer;
	struct vtm_buf             recvbuf;
	struct vtm_http_parser     parser;
	char                       *con_host;
	unsigned int               con_port;
	unsigned int               hints;
	unsigned long              opt_timeout;
};

/* forward declaration */
static int  vtm_http_client_con_open(vtm_http_client *cl, struct vtm_http_client_req *req, struct vtm_url *url);
static bool vtm_http_client_con_must_close(vtm_http_client *cl, struct vtm_http_client_req *req, struct vtm_http_client_res *res);
static void vtm_http_client_con_close(vtm_http_client *cl);
static int  vtm_http_client_send(vtm_http_client *cl, struct vtm_http_client_req *req, struct vtm_url *url);
static int  vtm_http_client_recv(vtm_http_client *cl, struct vtm_http_client_res *res);

vtm_http_client* vtm_http_client_new(void)
{
	vtm_http_client *cl;

	cl = malloc(sizeof(vtm_http_client));
	if (!cl) {
		vtm_err_oom();
		return NULL;
	}

	vtm_buf_init(&cl->recvbuf, VTM_BYTEORDER_LE);
	vtm_http_parser_init(&cl->parser, VTM_HTTP_PM_RESPONSE);

	cl->sock = NULL;
	cl->con_host = NULL;
	cl->con_port = 0;
	cl->hints = 0;
	cl->opt_timeout = 0;

	return cl;
}

void vtm_http_client_free(vtm_http_client *cl)
{
	if (!cl)
		return;

	vtm_buf_release(&cl->recvbuf);
	vtm_http_parser_release(&cl->parser);
	vtm_socket_free(cl->sock);

	free(cl->con_host);
	free(cl);
}

int vtm_http_client_take_socket(vtm_http_client *cl, vtm_socket **sock)
{
	if (!cl->sock)
		return VTM_E_INVALID_STATE;

	*sock = cl->sock;

	free(cl->con_host);

	cl->sock = NULL;
	cl->con_host = NULL;
	cl->con_port = 0;

	return VTM_OK;
}

int vtm_http_client_set_opt(vtm_http_client *cl, int opt, const void *val, size_t len)
{
	switch (opt) {
		case VTM_HTTP_CL_OPT_NO_CERT_CHECK:
			if (len != sizeof(bool))
				return VTM_E_INVALID_ARG;
			if (*((bool*) val))
				cl->hints |= VTM_HTTP_CL_HINT_NO_CERT_CHECK;
			else
				cl->hints &= ~VTM_HTTP_CL_HINT_NO_CERT_CHECK;
			return VTM_OK;

		case VTM_HTTP_CL_OPT_TIMEOUT:
			if (len != sizeof(unsigned long))
				return VTM_E_INVALID_ARG;
			cl->opt_timeout = *((unsigned long*) val);
			return VTM_OK;

		default:
			break;
	}

	return VTM_E_NOT_SUPPORTED;
}

int vtm_http_client_request(vtm_http_client *cl, struct vtm_http_client_req *req, struct vtm_http_client_res *res)
{
	int rc;
	struct vtm_url url;
	bool close_con;

	/* open or reuse connection */
	close_con = true;
	rc = vtm_http_client_con_open(cl, req, &url);
	if (rc != VTM_OK)
		goto end;

	/* send request */
	rc = vtm_http_client_send(cl, req, &url);
	if (rc != VTM_OK) {
		close_con = true;
		goto end;
	}

	/* receive response */
	rc = vtm_http_client_recv(cl, res);

	/* check if connection must be closed */
	close_con = (rc == VTM_OK) ? vtm_http_client_con_must_close(cl, req, res)
	                           : true;

end:
	if (close_con) {
		vtm_http_client_con_close(cl);
	}
	else if (!cl->con_host) {
		/* save connection addr */
		cl->con_host = url.host;
		cl->con_port = url.port;
		/* prevent saved host from beeing free'd */
		url.host = NULL;
	}

	vtm_url_release(&url);

	return rc;
}

static int vtm_http_client_con_open(vtm_http_client *cl, struct vtm_http_client_req *req, struct vtm_url *url)
{
	int rc;
	struct vtm_socket_tls_opts tls_opts;

	/* parse destination addr */
	rc = vtm_url_parse(req->url, url);
	if (rc != VTM_OK)
		return rc;

	/* already connected to destination addr? */
	if (cl->sock) {
		if (cl->con_port == url->port && strcmp(cl->con_host, url->host) == 0)
			return VTM_OK;
		vtm_http_client_con_close(cl);
	}

	/* open new connection */
	switch (url->scheme) {
		case VTM_URL_SCHEME_HTTP:
			cl->sock = vtm_socket_new(req->fam, VTM_SOCK_TYPE_STREAM);
			break;

		case VTM_URL_SCHEME_HTTPS:
			memset(&tls_opts, 0, sizeof(tls_opts));
			if (cl->hints & VTM_HTTP_CL_HINT_NO_CERT_CHECK)
				tls_opts.no_cert_check = true;
			cl->sock = vtm_socket_tls_new(req->fam, &tls_opts);
			break;
	}

	if (!cl->sock)
		return vtm_err_get_code();

	if (cl->opt_timeout > 0) {
		rc = vtm_socket_set_opt(cl->sock, VTM_SOCK_OPT_RECV_TIMEOUT,
			(unsigned long[]) {cl->opt_timeout}, sizeof(unsigned long));
		if (rc != VTM_OK)
			return rc;
	}

	return vtm_socket_connect(cl->sock, url->host, url->port);
}

static bool vtm_http_client_con_must_close(vtm_http_client *cl, struct vtm_http_client_req *req, struct vtm_http_client_res *res)
{
	const char *val;

	if (vtm_socket_get_state(cl->sock) & VTM_SOCK_STAT_CLOSED)
		return true;

	val = NULL;
	if (res->headers) {
		val = vtm_dataset_get_string(res->headers, VTM_HTTP_HEADER_CONNECTION);
		if (val && vtm_str_casecmp(val, VTM_HTTP_VALUE_CLOSE) == 0)
			return true;
	}

	if (req->version < VTM_HTTP_VER_1_1 &&
		(!val || vtm_str_casecmp(val, VTM_HTTP_VALUE_KEEP_ALIVE) != 0))
		return true;

	return false;
}

static void vtm_http_client_con_close(vtm_http_client *cl)
{
	if (cl->sock) {
		vtm_socket_close(cl->sock);
		vtm_socket_free(cl->sock);
		cl->sock = NULL;
	}

	free(cl->con_host);
	cl->con_host = NULL;
	cl->con_port = 0;
}

static int vtm_http_client_send(vtm_http_client *cl, struct vtm_http_client_req *req, struct vtm_url *url)
{
	const char *method;
	const char *version;

	/* send headers */
	method = VTM_HTTP_METHODS[req->method];
	version = VTM_HTTP_VERSIONS[req->version];

	cl->writer.sock = cl->sock;
	vtm_socket_writer_reset(&cl->writer);

	vtm_socket_writer_puts(&cl->writer, method);
	vtm_socket_writer_putc(&cl->writer, ' ');
	vtm_socket_writer_puts(&cl->writer, url->path);
	vtm_socket_writer_putc(&cl->writer, ' ');
	vtm_socket_writer_puts(&cl->writer, version);
	vtm_socket_writer_puts(&cl->writer, "\r\n");

	vtm_socket_writer_puts(&cl->writer, "Host: ");
	vtm_socket_writer_puts(&cl->writer, url->host);
	vtm_socket_writer_puts(&cl->writer, "\r\n");

	if (req->headers) {
		vtm_list *entries;
		struct vtm_dataset_entry *entry;
		size_t i, count;

		entries = vtm_dataset_entryset(req->headers);
		count = vtm_list_size(entries);
		for (i=0; i < count; i++) {
			entry = vtm_list_get_pointer(entries, i);

			vtm_socket_writer_puts(&cl->writer, entry->name);
			vtm_socket_writer_puts(&cl->writer, ": ");
			vtm_socket_writer_puts(&cl->writer, vtm_variant_as_str(entry->var));
			vtm_socket_writer_puts(&cl->writer, "\r\n");
		}

		vtm_list_free(entries);
	}

	vtm_socket_writer_puts(&cl->writer, "\r\n");

	/* send body */
	if (req->body && req->body_len > 0)
		vtm_socket_writer_putm(&cl->writer, req->body, req->body_len);

	/* flush buffer */
	return vtm_socket_writer_flush(&cl->writer);
}

static int vtm_http_client_recv(vtm_http_client *cl, struct vtm_http_client_res *res)
{
	int rc;
	size_t read;
	enum vtm_net_recv_stat stat;

	vtm_http_parser_reset(&cl->parser);
	vtm_buf_discard_processed(&cl->recvbuf);

	if (VTM_BUF_GET_AVAIL_TOTAL(&cl->recvbuf) > 0)
		goto parse;

	while (true) {
		rc = vtm_buf_ensure(&cl->recvbuf, 512);
		if (rc != VTM_OK)
			return VTM_NET_RECV_STAT_ERROR;

		rc = vtm_socket_read(cl->sock, VTM_BUF_PUT_PTR(&cl->recvbuf),
			VTM_BUF_PUT_AVAIL_TOTAL(&cl->recvbuf), &read);

		VTM_BUF_PUT_INC(&cl->recvbuf, read);
		if (rc != VTM_OK)
			return rc;

parse:
		stat = vtm_http_parser_run(&cl->parser, &cl->recvbuf);
		switch (stat) {
			case VTM_NET_RECV_STAT_AGAIN:
				continue;

			case VTM_NET_RECV_STAT_COMPLETE:
				res->version = cl->parser.version;
				res->status_code = cl->parser.res_status_code;
				res->status_msg = cl->parser.res_status_msg;
				res->headers = cl->parser.headers;
				res->body = cl->parser.body;
				res->body_len = cl->parser.body_len;

				vtm_http_parser_reset(&cl->parser);

				return VTM_OK;

			default:
				return VTM_ERROR;
		}
	}

	VTM_ABORT_NOT_REACHABLE;
	return VTM_ERROR;
}

void vtm_http_client_res_release(struct vtm_http_client_res *res)
{
	vtm_dataset_free(res->headers);
}
