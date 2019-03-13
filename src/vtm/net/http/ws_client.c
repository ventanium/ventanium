/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "ws_client.h"

#include <stdlib.h> /* malloc(), free() */
#include <string.h> /*memset() */
#include <vtm/core/buffer.h>
#include <vtm/core/error.h>
#include <vtm/core/hash.h>
#include <vtm/net/common.h>
#include <vtm/net/socket.h>
#include <vtm/net/http/http.h>
#include <vtm/net/http/http_client.h>
#include <vtm/net/http/http_client_intl.h>
#include <vtm/net/http/ws_frame_intl.h>
#include <vtm/net/http/ws_parser.h>
#include <vtm/util/base64.h>
#include <vtm/util/time.h>

#define VTM_WS_CL_HINT_NO_CERT_CHECK     1

struct vtm_ws_client
{
	vtm_socket            *sock;
	struct vtm_buf        recvbuf;
	struct vtm_ws_parser  parser;
	unsigned int          hints;
	long                  opt_timeout;
};

vtm_ws_client* vtm_ws_client_new(void)
{
	vtm_ws_client *cl;

	cl = malloc(sizeof(*cl));
	if (!cl) {
		vtm_err_oom();
		return NULL;
	}

	if (vtm_ws_parser_init(&cl->parser, VTM_WS_MODE_CLIENT) != VTM_OK) {
		free(cl);
		return NULL;
	}

	vtm_buf_init(&cl->recvbuf, VTM_NET_BYTEORDER);

	cl->sock = NULL;
	cl->hints = 0;
	cl->opt_timeout = 0;

	return cl;
}

void vtm_ws_client_free(vtm_ws_client *cl)
{
	if (!cl)
		return;

	if (cl->sock) {
		vtm_socket_close(cl->sock);
		vtm_socket_free(cl->sock);
	}

	vtm_ws_parser_release(&cl->parser);
	vtm_buf_release(&cl->recvbuf);

	free(cl);
}

int vtm_ws_client_set_opt(vtm_ws_client *cl, int opt, const void *val, size_t len)
{
	switch (opt) {
		case VTM_WS_CL_OPT_NO_CERT_CHECK:
			if (len != sizeof(bool))
				return VTM_E_INVALID_ARG;
			else if (*((bool*) val))
				cl->hints |= VTM_WS_CL_HINT_NO_CERT_CHECK;
			else
				cl->hints &= ~VTM_WS_CL_HINT_NO_CERT_CHECK;
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

int vtm_ws_client_connect(vtm_ws_client *cl, enum vtm_socket_family fam, const char *url)
{
	int rc, status;
	vtm_http_client *http_cl;
	struct vtm_http_client_req req;
	struct vtm_http_client_res res;
	vtm_dataset *headers;
	char ws_key[VTM_BASE64_ENC_BUF_LEN(sizeof(uint64_t))];
	uint64_t time;

	/* create http client */
	http_cl = vtm_http_client_new();
	if (!http_cl)
		return vtm_err_get_code();

	if (cl->hints & VTM_WS_CL_HINT_NO_CERT_CHECK) {
		rc = vtm_http_client_set_opt(http_cl, VTM_HTTP_CL_OPT_NO_CERT_CHECK,
			(bool[]) {true}, sizeof(bool));
		if (rc != VTM_OK)
			goto end;
	}

	if (cl->opt_timeout > 0) {
		rc = vtm_http_client_set_opt(http_cl, VTM_HTTP_CL_OPT_TIMEOUT,
			(unsigned long[]) {cl->opt_timeout}, sizeof(unsigned long));
		if (rc != VTM_OK)
			goto end;
	}

	/* generate websocket handshake key */
	time = vtm_time_current_micros();
	rc = vtm_base64_encode(&time, sizeof(time), ws_key, sizeof(ws_key));
	if (rc != VTM_OK)
		goto end;

	/* prepare headers */
	headers = vtm_dataset_new();
	if (!headers) {
		rc = vtm_err_get_code();
		goto end;
	}

	vtm_dataset_set_string(headers, VTM_HTTP_HEADER_UPGRADE, VTM_HTTP_VALUE_WEBSOCKET);
	vtm_dataset_set_string(headers, VTM_HTTP_HEADER_CONNECTION, VTM_HTTP_VALUE_UPGRADE);
	vtm_dataset_set_string(headers, VTM_HTTP_HEADER_SEC_WEBSOCKET_KEY, ws_key);
	vtm_dataset_set_string(headers, VTM_HTTP_HEADER_SEC_WEBSOCKET_VERSION, "13");

	/* prepare request */
	memset(&req, 0, sizeof(req));
	req.method = VTM_HTTP_METHOD_GET;
	req.version = VTM_HTTP_VER_1_1;
	req.fam = fam;
	req.headers = headers;
	req.url = url;
	req.body = NULL;
	req.body_len = 0;

	/* make request */
	rc = vtm_http_client_request(http_cl, &req, &res);
	if (rc != VTM_OK)
		goto end;

	/* check response */
	status = res.status_code;
	vtm_http_client_res_release(&res);
	if (status != VTM_HTTP_101_SWITCHING_PROTOCOLS) {
		rc = VTM_E_IO_UNKNOWN;
		goto end;
	}

	vtm_buf_clear(&cl->recvbuf);
	vtm_ws_parser_reset(&cl->parser);

	rc = vtm_http_client_take_socket(http_cl, &cl->sock);

end:
	vtm_http_client_free(http_cl);

	return rc;
}

int vtm_ws_client_close(vtm_ws_client *cl)
{
	if (!cl->sock)
		return VTM_E_INVALID_STATE;

	return vtm_socket_close(cl->sock);
}

int vtm_ws_client_send(vtm_ws_client *cl, enum vtm_ws_msg_type type, const void *src, size_t len)
{
	int rc;
	struct vtm_buf buf;
	struct vtm_ws_frame_desc desc;
	size_t payload_begin, written;
	if (!cl->sock)
		return VTM_E_INVALID_STATE;

	memset(&desc, 0, sizeof(desc));
	desc.fin = true;
	desc.opcode = type;
	desc.len = len;
	desc.mask = vtm_hash_unum(vtm_time_current_micros());
	desc.masked = 1;

	vtm_buf_init(&buf, VTM_BYTEORDER_LE);
	rc = vtm_ws_frame_write_header(&buf, &desc);
	if (rc != VTM_OK)
		goto end;

	payload_begin = buf.used;
	rc = vtm_buf_putm(&buf, src, len);
	if (rc != VTM_OK)
		goto end;

	vtm_ws_frame_mask_payload(buf.data + payload_begin, len, desc.mask);

	rc = vtm_socket_write(cl->sock, buf.data, buf.used, &written);
	if (rc == VTM_OK && written != buf.used)
		rc = VTM_E_IO_UNKNOWN;

end:
	vtm_buf_release(&buf);

	return rc;
}

int vtm_ws_client_recv(vtm_ws_client *cl, struct vtm_ws_msg *msg)
{
	int rc;
	enum vtm_net_recv_stat stat;
	size_t read;

	if (!cl->sock)
		return VTM_E_INVALID_STATE;

	if (VTM_BUF_GET_AVAIL_TOTAL(&cl->recvbuf) > 0)
		goto parse;

	while(true) {
		rc = vtm_buf_ensure(&cl->recvbuf, 512);
		if (rc != VTM_OK)
			return VTM_NET_RECV_STAT_ERROR;

		/* read from socket to buffer */
		rc = vtm_socket_read(cl->sock,
			VTM_BUF_PUT_PTR(&cl->recvbuf),
			VTM_BUF_PUT_AVAIL_TOTAL(&cl->recvbuf), &read);

		VTM_BUF_PUT_INC(&cl->recvbuf, read);
		if (rc != VTM_OK && rc != VTM_E_IO_AGAIN)
			return VTM_NET_RECV_STAT_ERROR;

parse:
		stat = vtm_ws_parser_run(&cl->parser, &cl->recvbuf);
		switch (stat) {
			case VTM_NET_RECV_STAT_COMPLETE:
				msg->con = NULL;
				return vtm_ws_parser_get_msg(&cl->parser, msg);

			case VTM_NET_RECV_STAT_AGAIN:
				continue;

			case VTM_NET_RECV_STAT_CLOSED:
				vtm_socket_free(cl->sock);
				cl->sock = NULL;
				return VTM_E_IO_CLOSED;

			case VTM_NET_RECV_STAT_INVALID:
			case VTM_NET_RECV_STAT_ERROR:
				return VTM_E_IO_UNKNOWN;
		}
	}

	return VTM_E_IO_UNKNOWN;
}
