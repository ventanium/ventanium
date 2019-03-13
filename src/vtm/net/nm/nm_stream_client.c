/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "nm_stream_client.h"

#include <vtm/core/error.h>
#include <vtm/core/buffer.h>
#include <vtm/net/common.h>
#include <vtm/net/socket.h>
#include <vtm/net/nm/nm_parser_intl.h>
#include <vtm/net/nm/nm_protocol_intl.h>

struct vtm_nm_stream_client
{
	vtm_socket            *sock;
	struct vtm_buf        buf;
	struct vtm_nm_parser  parser;
	unsigned long         opt_recv_timeout;
};

vtm_nm_stream_client* vtm_nm_stream_client_new(void)
{
	vtm_nm_stream_client *cl;

	cl = malloc(sizeof(*cl));
	if (!cl) {
		vtm_err_oom();
		return NULL;
	}

	cl->sock = NULL;
	cl->opt_recv_timeout = 0;

	vtm_buf_init(&cl->buf, VTM_NET_BYTEORDER);
	vtm_nm_parser_init(&cl->parser);

	return cl;
}

void vtm_nm_stream_client_free(vtm_nm_stream_client *cl)
{
	if (!cl)
		return;

	if (cl->sock)
		vtm_socket_free(cl->sock);

	vtm_buf_release(&cl->buf);
	vtm_nm_parser_release(&cl->parser);

	free(cl);
}

int vtm_nm_stream_client_set_opt(vtm_nm_stream_client *cl, int opt, const void *val, size_t len)
{
	switch (opt) {
		case VTM_NM_STREAM_CL_OPT_RECV_TIMEOUT:
			if (len != sizeof(unsigned long))
				return VTM_E_INVALID_ARG;
			cl->opt_recv_timeout = *((unsigned long*) val);
			return VTM_OK;

		default:
			break;
	}

	return VTM_E_NOT_SUPPORTED;
}

int vtm_nm_stream_client_connect(vtm_nm_stream_client *cl, struct vtm_nm_stream_client_opts *opts)
{
	int rc;
	struct vtm_socket_tls_opts tls_opts;

	if (cl->sock)
		return vtm_err_sets(VTM_E_INVALID_STATE, "Already connected\n");

	if (opts->tls.enabled) {
		tls_opts.is_server = false;
		tls_opts.no_cert_check = false;
		tls_opts.ca_file = opts->tls.cert_file;
		tls_opts.ciphers = opts->tls.ciphers;
		cl->sock = vtm_socket_tls_new(opts->addr.family, &tls_opts);
	}
	else {
		cl->sock = vtm_socket_new(opts->addr.family, VTM_SOCK_TYPE_STREAM);
	}

	if (!cl->sock)
		return vtm_err_get_code();

	if (cl->opt_recv_timeout > 0) {
		rc = vtm_socket_set_opt(cl->sock, VTM_SOCK_OPT_RECV_TIMEOUT,
			(unsigned long[]) {cl->opt_recv_timeout}, sizeof(unsigned long));
		if (rc != VTM_OK)
			return rc;
	}

	rc = vtm_socket_connect(cl->sock, opts->addr.host, opts->addr.port);
	if (rc == VTM_OK)
		return rc;

	vtm_socket_free(cl->sock);
	cl->sock = NULL;

	return rc;
}

int vtm_nm_stream_client_close(vtm_nm_stream_client *cl)
{
	if (!cl->sock)
		return vtm_err_sets(VTM_E_INVALID_STATE, "Not connected\n");

	vtm_socket_close(cl->sock);
	vtm_socket_free(cl->sock);
	cl->sock = NULL;

	return VTM_OK;
}

int vtm_nm_stream_client_send(vtm_nm_stream_client *cl, vtm_dataset *msg)
{
	int rc;
	struct vtm_buf buf;
	size_t written;

	vtm_buf_init(&buf, VTM_NET_BYTEORDER);

	rc = vtm_nm_msg_to_buf(msg, &buf);
	if (rc != VTM_OK)
		goto end;

	rc = vtm_socket_write(cl->sock, buf.data, buf.used, &written);
	if (rc != VTM_OK)
		goto end;

	if (written != buf.used)
		rc = VTM_E_IO_PARTIAL;

end:
	vtm_buf_release(&buf);

	return rc;
}

int vtm_nm_stream_client_recv(vtm_nm_stream_client *cl, vtm_dataset *msg)
{
	int rc;
	size_t read;
	enum vtm_net_recv_stat stat;

	vtm_nm_parser_reset(&cl->parser);
	cl->parser.msg = msg;

	if (VTM_BUF_GET_AVAIL_TOTAL(&cl->buf) > 0)
		goto parse;

	while (true) {
		rc = vtm_buf_ensure(&cl->buf, 512);
		if (rc != VTM_OK)
			return VTM_NET_RECV_STAT_ERROR;

		rc = vtm_socket_read(cl->sock, VTM_BUF_PUT_PTR(&cl->buf),
			VTM_BUF_PUT_AVAIL_TOTAL(&cl->buf), &read);

		VTM_BUF_PUT_INC(&cl->buf, read);
		if (rc != VTM_OK)
			goto end;
parse:
		stat = vtm_nm_parser_run(&cl->parser, &cl->buf);
		switch (stat) {
			case VTM_NET_RECV_STAT_AGAIN:
				continue;

			case VTM_NET_RECV_STAT_COMPLETE:
				vtm_buf_discard_processed(&cl->buf);
				rc = VTM_OK;
				goto end;

			default:
				rc = VTM_ERROR;
				goto end;
		}
	}

end:
	cl->parser.msg = NULL;

	return rc;
}
