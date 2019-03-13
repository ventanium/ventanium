/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "http_parser.h"

#include <stdlib.h> /* free() */
#include <string.h> /* memmove() */
#include <ctype.h>
#include <vtm/core/error.h>
#include <vtm/core/lang.h>
#include <vtm/core/string.h>

#define VTM_HTTP_PARSER_MARK_BODY(PAR, BUF)                  \
	do {                                                     \
		if ((PAR)->body_len > 0)                             \
			(PAR)->body = (BUF)->data + (PAR)->body_begin;   \
	} while (0)

/* forward declaration */
static VTM_INLINE bool vtm_http_parser_isspace(char c);
static VTM_INLINE int  vtm_http_parser_save_header(struct vtm_http_parser *par, struct vtm_buf *buf);

void vtm_http_parser_init(struct vtm_http_parser *par, enum vtm_http_parser_mode mode)
{
	par->mode = mode;

	par->max_header_size = VTM_HTTP_DEF_MAX_HEADER_SIZE;
	par->max_body_size = VTM_HTTP_DEF_MAX_BODY_SIZE;

	vtm_http_parser_reset(par);
}

void vtm_http_parser_release(struct vtm_http_parser *par)
{
	if (par->body)
		free(par->body);

	if (par->headers_free && par->headers)
		vtm_dataset_free(par->headers);

	if (par->req_params_free && par->req_params)
		vtm_dataset_free(par->req_params);
}

void vtm_http_parser_reset(struct vtm_http_parser *par)
{
	par->state = VTM_HTTP_PARSE_BEGIN;
	par->state_chars = 0;

	par->req_method = 0;
	par->req_path = NULL;
	par->req_params = NULL;
	par->req_params_free = false;
	par->res_status_code = 0;
	par->res_status_msg = NULL;

	par->version = 0;
	par->version_major = 0;
	par->version_minor = 0;
	par->headers = NULL;
	par->headers_free = false;
	par->body = NULL;
	par->body_len = 0;

	par->status_msg_begin = 0;
	par->path_begin = 0;
	par->param_name_begin = 0;

	par->header_name_begin = 0;
	par->header_value_begin = 0;

	par->body_begin = 0;
	par->chunk_dst = 0;
	par->chunk_begin = 0;
	par->chunk_size = 0;
}

enum vtm_net_recv_stat vtm_http_parser_run(struct vtm_http_parser *par, struct vtm_buf *buf)
{
	int rc;
	char c;
	size_t i, j, n;
	enum vtm_http_parser_state oldstate;
	const char *val;
	bool found;

	oldstate = par->state;
	n = VTM_BUF_GET_AVAIL_TOTAL(buf);

	if (n == 0) {
		switch (par->state) {
			case VTM_HTTP_PARSE_BODY_READALL:
				par->body_len = buf->used - par->body_begin;
				VTM_HTTP_PARSER_MARK_BODY(par, buf);
				par->state = VTM_HTTP_PARSE_COMPLETE;
				return VTM_NET_RECV_STAT_COMPLETE;

			default:
				return VTM_NET_RECV_STAT_AGAIN;
		}
	}

	for (i=0; i < n; i++) {

		/* check header size limit */
		if (VTM_UNLIKELY(par->state < VTM_HTTP_PARSE_BODY &&
			buf->read > par->max_header_size)) {
			return VTM_NET_RECV_STAT_INVALID;
		}

		/* check body size limit */
		if (VTM_UNLIKELY(par->state >= VTM_HTTP_PARSE_BODY &&
			buf->read > par->max_body_size)) {
			return VTM_NET_RECV_STAT_INVALID;
		}

		c = VTM_BUF_GETC(buf);

eval:
		if (par->state != oldstate) {
			oldstate = par->state;
			par->state_chars = 0;
		}
		par->state_chars++;

		switch (par->state) {
			case VTM_HTTP_PARSE_BEGIN:
				switch (par->mode) {
					case VTM_HTTP_PM_REQUEST:
						par->state = VTM_HTTP_PARSE_REQ_METHOD;
						break;

					case VTM_HTTP_PM_RESPONSE:
						par->state = VTM_HTTP_PARSE_VERSION_H;
						break;
				}
				goto eval;

			case VTM_HTTP_PARSE_REQ_METHOD:
				if (c != ' ')
					continue;

				buf->data[buf->read-1] = '\0';
				rc = vtm_http_get_method((char*) buf->data, &par->req_method);
				if (rc != VTM_OK)
					return VTM_NET_RECV_STAT_INVALID;
				par->state = VTM_HTTP_PARSE_REQ_PATH;
				break;

			case VTM_HTTP_PARSE_REQ_PATH:
				switch (c) {
					case ' ':
						par->state = VTM_HTTP_PARSE_VERSION_H;
						break;

					case '?':
						par->state = VTM_HTTP_PARSE_REQ_PARAM_BEGIN;
						break;

					default:
						continue;
				}
				buf->data[buf->read-1] = '\0';
				par->req_path = (char*) buf->data + buf->read - par->state_chars;
				break;

			case VTM_HTTP_PARSE_REQ_PARAM_BEGIN:
				par->param_name_begin = 0;
				par->state = VTM_HTTP_PARSE_REQ_PARAM_NAME;
				goto eval;

			case VTM_HTTP_PARSE_REQ_PARAM_NAME:
				switch (c) {
					case ' ':
						par->state = VTM_HTTP_PARSE_VERSION_H;
						continue;

					case '&':
						par->state = VTM_HTTP_PARSE_REQ_PARAM_BEGIN;
						continue;

					case '=':
						if (par->state_chars > 1) {
							buf->data[buf->read-1] = '\0';
							par->param_name_begin = buf->read - par->state_chars;
						}
						par->state = VTM_HTTP_PARSE_REQ_PARAM_VALUE;
						continue;

					default:
						continue;
				}
				break;

			case VTM_HTTP_PARSE_REQ_PARAM_VALUE:
				switch (c) {
					case ' ':
						par->state = VTM_HTTP_PARSE_VERSION_H;
						break;

					case '&':
						par->state = VTM_HTTP_PARSE_REQ_PARAM_BEGIN;
						break;

					default:
						continue;
				}
				if (par->param_name_begin == 0)
					continue;

				buf->data[buf->read-1] = '\0';
				if (!par->req_params) {
					par->req_params = vtm_dataset_new();
					if (!par->req_params)
						return VTM_NET_RECV_STAT_ERROR;
				}
				vtm_dataset_set_string(par->req_params,
					(char*) buf->data + par->param_name_begin,
					(char*) buf->data + buf->read - par->state_chars);
				break;

			case VTM_HTTP_PARSE_REQ_LINE_LF:
				if (VTM_UNLIKELY(c != '\n'))
					return VTM_NET_RECV_STAT_INVALID;
				par->state = VTM_HTTP_PARSE_HEADER_LINE_BEGIN;
				break;

			case VTM_HTTP_PARSE_RES_STATUS_CODE:
				/* digits 1-3 */
				if (par->state_chars <= 3) {
					if (VTM_UNLIKELY(!isdigit(c)))
						return VTM_NET_RECV_STAT_INVALID;
					par->res_status_code = par->res_status_code * 10 + (c - '0');
					continue;
				}

				/* space */
				if (VTM_UNLIKELY(c != ' '))
					return VTM_NET_RECV_STAT_INVALID;

				par->status_msg_begin = buf->read;
				par->state = VTM_HTTP_PARSE_RES_STATUS_MSG_CR;
				break;

			case VTM_HTTP_PARSE_RES_STATUS_MSG_CR:
				if (c != '\r')
					continue;

				buf->data[buf->read-1] = '\0';
				par->res_status_msg = (char*) buf->data + par->status_msg_begin;
				par->state = VTM_HTTP_PARSE_RES_STATUS_MSG_LF;
				break;

			case VTM_HTTP_PARSE_RES_STATUS_MSG_LF:
				if (VTM_UNLIKELY(c != '\n'))
					return VTM_NET_RECV_STAT_INVALID;
				par->header_name_begin = buf->read;
				par->state = VTM_HTTP_PARSE_HEADER_LINE_BEGIN;
				break;

			case VTM_HTTP_PARSE_VERSION_H:
				if (VTM_UNLIKELY(c != 'H'))
					return VTM_NET_RECV_STAT_INVALID;
				par->state = VTM_HTTP_PARSE_VERSION_HT;
				break;

			case VTM_HTTP_PARSE_VERSION_HT:
				if (VTM_UNLIKELY(c != 'T'))
					return VTM_NET_RECV_STAT_INVALID;
				par->state = VTM_HTTP_PARSE_VERSION_HTT;
				break;

			case VTM_HTTP_PARSE_VERSION_HTT:
				if (VTM_UNLIKELY(c != 'T'))
					return VTM_NET_RECV_STAT_INVALID;
				par->state = VTM_HTTP_PARSE_VERSION_HTTP;
				break;

			case VTM_HTTP_PARSE_VERSION_HTTP:
				if (VTM_UNLIKELY(c != 'P'))
					return VTM_NET_RECV_STAT_INVALID;
				par->state = VTM_HTTP_PARSE_VERSION_SLASH;
				break;

			case VTM_HTTP_PARSE_VERSION_SLASH:
				if (VTM_UNLIKELY(c != '/'))
					return VTM_NET_RECV_STAT_INVALID;
				par->state = VTM_HTTP_PARSE_VERSION_MAJOR;
				break;

			case VTM_HTTP_PARSE_VERSION_MAJOR:
				if (isdigit(c)) {
					par->version_major = par->version_major * 10 + (c-'0');
					continue;
				}
				if (VTM_UNLIKELY(c != '.'))
					return VTM_NET_RECV_STAT_INVALID;
				par->state = VTM_HTTP_PARSE_VERSION_MINOR;
				break;

			case VTM_HTTP_PARSE_VERSION_MINOR:
				if (isdigit(c)) {
					par->version_minor = par->version_minor * 10 + (c-'0');
					continue;
				}
				switch (par->mode) {
					case VTM_HTTP_PM_REQUEST:
						if (VTM_UNLIKELY(c != '\r'))
							return VTM_NET_RECV_STAT_INVALID;
						par->state = VTM_HTTP_PARSE_REQ_LINE_LF;
						break;
					case VTM_HTTP_PM_RESPONSE:
						if (VTM_UNLIKELY(c != ' '))
							return VTM_NET_RECV_STAT_INVALID;
						par->state = VTM_HTTP_PARSE_RES_STATUS_CODE;
						break;
				}
				rc = vtm_http_get_version(par->version_major,
					par->version_minor,
					&par->version);
				if (rc != VTM_OK)
					return VTM_NET_RECV_STAT_INVALID;
				break;

			case VTM_HTTP_PARSE_HEADER_LINE_BEGIN:
				par->header_name_begin = buf->read-1;
				par->header_value_begin = 0;
				par->state = VTM_HTTP_PARSE_HEADER_NAME;
				goto eval;

			case VTM_HTTP_PARSE_HEADER_NAME:
				switch (c) {
					case '\r':
						if (par->state_chars == 1) {
							par->state = VTM_HTTP_PARSE_HEADERS_END_LF;
							continue;
						}
						/* fallthrough */

					case ' ':
					case '\n':
						return VTM_NET_RECV_STAT_INVALID;

					case ':':
						break;

					default:
						continue;
				}
				buf->data[buf->read-1] = '\0';
				par->state = VTM_HTTP_PARSE_HEADER_VALUE;
				break;

			case VTM_HTTP_PARSE_HEADER_VALUE:
				if (vtm_http_parser_isspace(c) && par->header_value_begin == 0)
					continue;

				switch (c) {
					case '\r':
						if (par->header_value_begin == 0)
							return VTM_NET_RECV_STAT_INVALID;
						break;

					case '\n':
						return VTM_NET_RECV_STAT_INVALID;

					default:
						if (par->header_value_begin == 0)
							par->header_value_begin = buf->read-1;
						continue;
				}
				j = buf->read-1;
				while (vtm_http_parser_isspace(buf->data[j]))
					j--;
				buf->data[j] = '\0';
				par->state = VTM_HTTP_PARSE_HEADER_LINE_LF;
				break;

			case VTM_HTTP_PARSE_HEADER_LINE_LF:
				if (VTM_UNLIKELY(c != '\n'))
					return VTM_NET_RECV_STAT_INVALID;
				par->state = VTM_HTTP_PARSE_HEADER_LINE_COMPLETE;
				goto eval;

			case VTM_HTTP_PARSE_HEADER_LINE_COMPLETE:
				if (!par->headers) {
					par->headers = vtm_dataset_newh(VTM_DS_HINT_IGNORE_CASE);
					if (!par->headers)
						return VTM_NET_RECV_STAT_ERROR;
					par->headers_free = true;
				}
				rc = vtm_http_parser_save_header(par, buf);
				if (rc != VTM_OK)
					return VTM_NET_RECV_STAT_ERROR;

				par->state = VTM_HTTP_PARSE_HEADER_LINE_BEGIN;
				break;

			case VTM_HTTP_PARSE_HEADERS_END_LF:
				if (VTM_UNLIKELY(c != '\n'))
					return VTM_NET_RECV_STAT_INVALID;
				par->state = VTM_HTTP_PARSE_BODY;
				goto eval;

			case VTM_HTTP_PARSE_BODY:
				par->body_begin = buf->read;

				if (par->headers) {
					/* Transfer-Encoding given? */
					val = vtm_dataset_get_string(par->headers,
						VTM_HTTP_HEADER_TRANSFER_ENCODING);

					if (val && vtm_str_list_contains(val, ",",
							VTM_HTTP_VALUE_CHUNKED, true)) {
						par->state = VTM_HTTP_PARSE_BODY_CHUNKED;
						continue;
					}

					/* Content-Length given? */
					if (vtm_dataset_contains(par->headers,
						 VTM_HTTP_HEADER_CONTENT_LENGTH)) {
						par->body_len = vtm_dataset_get_uint64(par->headers,
							VTM_HTTP_HEADER_CONTENT_LENGTH);
						par->state = VTM_HTTP_PARSE_BODY_FIXEDLENGTH;
						goto eval;
					}
				}

				switch (par->mode) {
					case VTM_HTTP_PM_REQUEST:
						par->state = VTM_HTTP_PARSE_COMPLETE;
						goto eval;

					case VTM_HTTP_PM_RESPONSE:
						/* read until connection closed */
						par->state = VTM_HTTP_PARSE_BODY_READALL;
						goto eval;
				}
				break;

			case VTM_HTTP_PARSE_BODY_READALL:
				VTM_BUF_PROCESS_ALL(buf);
				if (buf->read > par->max_body_size)
					return VTM_NET_RECV_STAT_INVALID;
				return VTM_NET_RECV_STAT_AGAIN;

			case VTM_HTTP_PARSE_BODY_FIXEDLENGTH:
				if (par->state_chars < par->body_len)
					continue;
				par->state = VTM_HTTP_PARSE_COMPLETE;
				goto eval;

			case VTM_HTTP_PARSE_BODY_CHUNKED:
				if (par->chunk_dst == 0)
					par->chunk_dst = par->body_begin;
				par->chunk_size = 0;
				par->state = VTM_HTTP_PARSE_BODY_CHUNK_SIZE;
				goto eval;

			case VTM_HTTP_PARSE_BODY_CHUNK_SIZE:
				if (c == '\r') {
					par->state = VTM_HTTP_PARSE_BODY_CHUNK_SIZE_LF;
					continue;
				}

				found = false;
				if (c >= '0' && c <= '9') {
					c = c - '0';
					found = true;
				}
				else if (c >= 'A' && c <= 'F') {
					c = c - 'A' + 10;
					found = true;
				}
				else if (c >= 'a' && c <= 'f') {
					c = c - 'a' + 10;
					found = true;
				}

				if (!found)
					return VTM_NET_RECV_STAT_INVALID;

				par->chunk_size = (par->chunk_size << 4) + c;
				break;

			case VTM_HTTP_PARSE_BODY_CHUNK_SIZE_LF:
				if (VTM_UNLIKELY(c != '\n'))
					return VTM_NET_RECV_STAT_INVALID;
				par->chunk_begin = buf->read;
				par->state = VTM_HTTP_PARSE_BODY_CHUNK_CONTENT;
				break;

			case VTM_HTTP_PARSE_BODY_CHUNK_CONTENT:
				if (par->state_chars <= par->chunk_size)
					continue;

				if (VTM_UNLIKELY(c != '\r'))
					return VTM_NET_RECV_STAT_INVALID;

				par->state = VTM_HTTP_PARSE_BODY_CHUNK_END_LF;
				break;

			case VTM_HTTP_PARSE_BODY_CHUNK_END_LF:
				if (VTM_UNLIKELY(c != '\n'))
					return VTM_NET_RECV_STAT_INVALID;

				if (par->chunk_size == 0) {
					par->state = VTM_HTTP_PARSE_COMPLETE;
					goto eval;
				}

				memmove(buf->data + par->chunk_dst,
					buf->data + par->chunk_begin,
					par->chunk_size);

				par->chunk_dst += par->chunk_size;
				par->body_len += par->chunk_size;
				par->state = VTM_HTTP_PARSE_BODY_CHUNKED;
				break;

			case VTM_HTTP_PARSE_COMPLETE:
				VTM_HTTP_PARSER_MARK_BODY(par, buf);
				return VTM_NET_RECV_STAT_COMPLETE;
		}
	}
	return VTM_NET_RECV_STAT_AGAIN;
}

static VTM_INLINE bool vtm_http_parser_isspace(char c)
{
	switch (c) {
		case ' ':
		case '\t':
			return true;
	}

	return false;
}

static VTM_INLINE int vtm_http_parser_save_header(struct vtm_http_parser *par, struct vtm_buf *buf)
{
	const char *prev;
	char *merged, *value;
	size_t prev_len, value_len;

	value = (char*) buf->data + par->header_value_begin;
	prev = vtm_dataset_get_string(par->headers, (char*) buf->data + par->header_name_begin);
	if (prev) {
		prev_len = strlen(prev);
		value_len = strlen(value);
		merged = malloc(prev_len + value_len + 3);
		if (!merged) {
			vtm_err_oom();
			return vtm_err_get_code();
		}

		memcpy(merged, prev, prev_len);
		merged[prev_len]   = ',';
		merged[prev_len+1] = ' ';
		memcpy(merged + prev_len + 2, value, value_len);
		merged[prev_len + value_len + 2] = '\0';
	}
	else {
		merged = value;
	}

	vtm_dataset_set_string(par->headers, (char*) buf->data + par->header_name_begin, merged);

	return VTM_OK;
}
