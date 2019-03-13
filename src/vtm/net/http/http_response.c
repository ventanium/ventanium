/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "http_response.h"

#include <string.h> /* strlen() */
#include <vtm/core/buffer.h>
#include <vtm/core/error.h>
#include <vtm/core/format.h>
#include <vtm/core/string.h>
#include <vtm/core/version.h>
#include <vtm/net/socket_emitter.h>
#include <vtm/net/http/http_format.h>
#include <vtm/net/http/http_response_intl.h>

enum vtm_http_res_stage
{
	VTM_HTTP_RES_STAGE_UNINITIALZED,
	VTM_HTTP_RES_STAGE_HEADER_OR_BODY,
	VTM_HTTP_RES_STAGE_HEADER,
	VTM_HTTP_RES_STAGE_BODY,
	VTM_HTTP_RES_STAGE_COMPLETED
};

struct vtm_http_res
{
	vtm_http_con *con;

	enum vtm_http_version version;
	enum vtm_http_res_mode mode;
	enum vtm_http_res_stage stage;
	enum vtm_http_res_act act;
	void *act_data;

	struct vtm_buf buf;
	struct vtm_buf body_buf;
	struct vtm_socket_emitter *body_se;
};

/* forward declaration */
static int vtm_http_res_write_chunked(vtm_http_res *res, const void *src, size_t len);
static int vtm_http_res_close_headers(vtm_http_res *res);
static int vtm_http_res_send(vtm_http_res *res);

vtm_http_res* vtm_http_res_new(void)
{
	vtm_http_res *res;

	res = malloc(sizeof(vtm_http_res));
	if (!res) {
		vtm_err_oom();
		return NULL;
	}

	res->con = NULL;
	res->version = 0;

	res->mode = VTM_HTTP_RES_MODE_FIXED;
	res->stage = VTM_HTTP_RES_STAGE_UNINITIALZED;
	res->act = VTM_HTTP_RES_ACT_CLOSE_CON;

	vtm_buf_init(&res->buf, vtm_sys_get_byteorder());
	vtm_buf_init(&res->body_buf, vtm_sys_get_byteorder());

	res->body_se = NULL;

	return res;
}

void vtm_http_res_free(vtm_http_res *res)
{
	vtm_buf_release(&res->buf);
	vtm_buf_release(&res->body_buf);
	free(res);
}

void vtm_http_res_prepare(vtm_http_res *res, struct vtm_http_req *req)
{
	const char *val;

	res->con = req->con;
	res->version = req->version;

	res->mode = VTM_HTTP_RES_MODE_FIXED;
	res->stage = VTM_HTTP_RES_STAGE_UNINITIALZED;

	vtm_buf_clear(&res->buf);
	vtm_buf_clear(&res->body_buf);
	res->body_se = NULL;

	/* eval default action */
	res->act_data = NULL;
	switch (req->version) {
		case VTM_HTTP_VER_1_0:
			res->act = VTM_HTTP_RES_ACT_CLOSE_CON;
			break;

		case VTM_HTTP_VER_1_1:
			val = vtm_http_req_get_header_str(req, VTM_HTTP_HEADER_CONNECTION);
			if (val && vtm_str_casecmp(val, VTM_HTTP_VALUE_CLOSE) == 0)
				res->act = VTM_HTTP_RES_ACT_CLOSE_CON;
			else
				res->act = VTM_HTTP_RES_ACT_KEEP_CON;
			break;
	}
}

int vtm_http_res_begin(vtm_http_res *res, enum vtm_http_res_mode mode, int status)
{
	int rc;
	const char *version;
	const char *reason;
	char status_str[VTM_FMT_CHARS_INT32];

	if (res->stage != VTM_HTTP_RES_STAGE_UNINITIALZED)
		return VTM_ERROR;

	res->stage = VTM_HTTP_RES_STAGE_HEADER_OR_BODY;
	res->mode = mode;

	version = vtm_http_get_version_string(res->version);
	reason = vtm_http_get_status_phrase(status);

	rc = vtm_fmt_int(status_str, status);
	status_str[rc] = '\0';

	vtm_buf_puts(&res->buf, version);
	vtm_buf_putc(&res->buf, ' ');
	vtm_buf_puts(&res->buf, status_str);
	vtm_buf_putc(&res->buf, ' ');
	vtm_buf_puts(&res->buf, reason);
	vtm_buf_puts(&res->buf, "\r\n");

	if (res->buf.err != VTM_OK)
		return VTM_ERROR;

	rc = vtm_http_res_header(res, VTM_HTTP_HEADER_SERVER, VTM_BUILD_VERSION);
	if (rc != VTM_OK)
		return rc;

	rc = vtm_http_res_set_date(res);
	if (rc != VTM_OK)
		return rc;

	switch (res->mode) {
		case VTM_HTTP_RES_MODE_CHUNKED:
			rc = vtm_http_res_header(res, VTM_HTTP_HEADER_TRANSFER_ENCODING, VTM_HTTP_VALUE_CHUNKED);
			break;

		default:
			break;
	}

	return rc;
}

int vtm_http_res_header(vtm_http_res *res, const char *name, const char *value)
{
	if (res->stage != VTM_HTTP_RES_STAGE_HEADER_OR_BODY &&
		res->stage != VTM_HTTP_RES_STAGE_HEADER)
		return VTM_ERROR;

	res->stage = VTM_HTTP_RES_STAGE_HEADER;

	vtm_buf_puts(&res->buf, name);
	vtm_buf_puts(&res->buf, ": ");
	vtm_buf_puts(&res->buf, value);
	vtm_buf_puts(&res->buf, "\r\n");

	return (res->buf.err);
}

static int vtm_http_res_close_headers(vtm_http_res *res)
{
	switch (res->act) {
		case VTM_HTTP_RES_ACT_CLOSE_CON:
			vtm_http_res_header(res, VTM_HTTP_HEADER_CONNECTION, VTM_HTTP_VALUE_CLOSE);
			break;

		case VTM_HTTP_RES_ACT_KEEP_CON:
			vtm_http_res_header(res, VTM_HTTP_HEADER_CONNECTION, VTM_HTTP_VALUE_KEEP_ALIVE);
			break;

		default:
			break;
	}

	return vtm_buf_puts(&res->buf, "\r\n");
}

int vtm_http_res_body_str(vtm_http_res *res, const char *data)
{
	return vtm_http_res_body_raw(res, data, strlen(data));
}

int vtm_http_res_body_raw(vtm_http_res *res, const char *src, size_t len)
{
	int rc;

	if (res->stage == VTM_HTTP_RES_STAGE_UNINITIALZED ||
		res->stage == VTM_HTTP_RES_STAGE_COMPLETED)
		return VTM_ERROR;

	switch (res->mode) {
		case VTM_HTTP_RES_MODE_FIXED:
			return vtm_buf_putm(&res->body_buf, src, len);

		case VTM_HTTP_RES_MODE_CHUNKED:
			if (res->stage != VTM_HTTP_RES_STAGE_BODY) {
				rc = vtm_http_res_close_headers(res);
				res->stage = VTM_HTTP_RES_STAGE_BODY;
				if (rc != VTM_OK)
					return rc;
			}
			return vtm_http_res_write_chunked(res, src, len);
	}

	VTM_ABORT_NOT_REACHABLE;
	return VTM_ERROR;
}

int vtm_http_res_body_emt(vtm_http_res *res, struct vtm_socket_emitter *se)
{
	if (res->body_se)
		return VTM_ERROR;

	res->body_se = se;

	return VTM_OK;
}

static int vtm_http_res_write_chunked(vtm_http_res *res, const void *src, size_t len)
{
	int rc;
	int hex_len;

	hex_len = vtm_fmt_hex_size(NULL, len);
	rc = vtm_buf_ensure(&res->buf, hex_len);
	if (rc != VTM_OK)
		return rc;

	vtm_fmt_hex_size((char*) VTM_BUF_PUT_PTR(&res->buf), len);
	VTM_BUF_PUT_INC(&res->buf, hex_len);

	vtm_buf_puts(&res->buf, "\r\n");
	vtm_buf_putm(&res->buf, src, len);
	vtm_buf_puts(&res->buf, "\r\n");

	return res->buf.err;
}

int vtm_http_res_set_action(vtm_http_res *res, enum vtm_http_res_act act, void *data)
{
	if (res->stage >= VTM_HTTP_RES_STAGE_BODY)
		return VTM_E_INVALID_STATE;

	res->act = act;
	res->act_data = data;

	return VTM_OK;
}

enum vtm_http_res_act vtm_http_res_get_action(vtm_http_res *res)
{
	return res->act;
}

void* vtm_http_res_get_action_data(vtm_http_res *res)
{
	return res->act_data;
}

int vtm_http_res_end(vtm_http_res *res)
{
	int rc;
	char *val;
	uint64_t len;
	uint64_t chain_len;

	if (res->stage == VTM_HTTP_RES_STAGE_UNINITIALZED ||
		res->stage == VTM_HTTP_RES_STAGE_COMPLETED)
		return VTM_ERROR;

	rc = VTM_OK;

	switch (res->mode) {
		case VTM_HTTP_RES_MODE_FIXED:
			len = 0;
			if (res->body_buf.used > 0)
				len += res->body_buf.used;

			if (res->body_se) {
				rc = vtm_socket_emitter_get_chain_lensum(res->body_se, &chain_len);
				if (rc != VTM_OK)
					return rc;
				len += chain_len;
			}

			val = vtm_str_printf("%llu", len);
			rc = vtm_http_res_header(res, VTM_HTTP_HEADER_CONTENT_LENGTH, val);
			free(val);
			if (rc != VTM_OK)
				return rc;

			rc = vtm_http_res_close_headers(res);
			if (rc != VTM_OK)
				return rc;

			if (res->body_buf.used > 0)
				vtm_buf_putm(&res->buf, res->body_buf.data, res->body_buf.used);
			break;

		case VTM_HTTP_RES_MODE_CHUNKED:
			if (res->stage == VTM_HTTP_RES_STAGE_HEADER_OR_BODY ||
				res->stage == VTM_HTTP_RES_STAGE_HEADER) {
				rc = vtm_http_res_close_headers(res);
				if (rc != VTM_OK)
					return rc;
			}
			rc = vtm_http_res_write_chunked(res, NULL, 0);
			break;
	}

	if (rc != VTM_OK)
		return rc;

	return vtm_http_res_send(res);
}

static int vtm_http_res_send(vtm_http_res *res)
{
	int rc;
	vtm_socket *sock;
	struct vtm_socket_emitter *se;

	sock = vtm_http_con_get_socket(res->con);

	se = vtm_socket_emitter_for_buffer(sock, &res->buf, false);
	if (!se)
		return vtm_err_get_code();

	if (res->body_se) {
		res->body_se->sock = sock;
		se->next = res->body_se;
	}

	rc = vtm_socket_emitter_try_write(&se);
	switch (rc) {
		case VTM_OK:
			res->stage = VTM_HTTP_RES_STAGE_COMPLETED;
			return VTM_OK;

		case VTM_E_IO_AGAIN:
			res->stage = VTM_HTTP_RES_STAGE_COMPLETED;
			vtm_http_con_set_emitter(res->con, se);
			return VTM_OK;

		default:
			vtm_socket_emitter_free_chain(se);
			break;
	}
	return rc;
}

int vtm_http_res_set_date(vtm_http_res *res)
{
	int rc;
	char buf[VTM_HTTP_DATE_LEN];
	struct vtm_date now;

	rc = vtm_date_now_utc(&now);
	if (rc != VTM_OK)
		return rc;

	rc = vtm_http_fmt_date(buf, sizeof(buf), &now);
	if (rc != VTM_OK)
		return rc;

	return vtm_http_res_header(res, VTM_HTTP_HEADER_DATE, buf);
}

bool vtm_http_res_was_started(vtm_http_res *res)
{
	return res->stage != VTM_HTTP_RES_STAGE_UNINITIALZED;
}

bool vtm_http_res_was_sent(vtm_http_res *res)
{
	return res->stage == VTM_HTTP_RES_STAGE_COMPLETED;
}

enum vtm_http_version vtm_http_res_get_version(vtm_http_res *res)
{
	return res->version;
}
