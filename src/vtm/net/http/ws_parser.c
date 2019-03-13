/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "ws_parser.h"

#include <string.h> /* memcpy(), memset(), memmove() */
#include <vtm/core/error.h>
#include <vtm/core/types.h>
#include <vtm/net/http/ws.h>
#include <vtm/net/http/ws_error.h>
#include <vtm/net/http/ws_frame_intl.h>
#include <vtm/net/http/ws_message_intl.h>

#define VTM_WS_PARSER_MSG_BUF_INIT       1024
#define VTM_WS_PARSER_MSG_BUF_MAX       65536

/* forward declaration */
static bool vtm_ws_parser_is_valid_opcode(unsigned int opcode);
static bool vtm_ws_parser_is_control_opcode(unsigned int opcode);
static enum vtm_ws_msg_type vtm_ws_parser_convert_opcode(unsigned int opcode);

int vtm_ws_parser_init(struct vtm_ws_parser *par, enum vtm_ws_mode mode)
{
	par->msg_buf = malloc(VTM_WS_PARSER_MSG_BUF_INIT);
	if (!par->msg_buf) {
		vtm_err_oom();
		return vtm_err_get_code();
	}

	par->mode = mode;
	par->stage = VTM_WS_PARSE_MSG_BEGIN;
	par->msg_buf_size = VTM_WS_PARSER_MSG_BUF_INIT;
	par->msg_buf_used = 0;
	par->has_ctrl_msg = false;

	return VTM_OK;
}

void vtm_ws_parser_release(struct vtm_ws_parser *par)
{
	free(par->msg_buf);
}

void vtm_ws_parser_reset(struct vtm_ws_parser *par)
{
	par->stage = VTM_WS_PARSE_MSG_BEGIN;
	par->msg_buf_used = 0;

	if (par->has_ctrl_msg) {
		par->has_ctrl_msg = false;
		vtm_ws_msg_release(&par->ctrl_msg);
	}
}

enum vtm_net_recv_stat vtm_ws_parser_run(struct vtm_ws_parser *par, struct vtm_buf *buf)
{
	int rc;
	enum vtm_net_recv_stat stat;
	unsigned char c, *payload;
	size_t i;
	uint64_t payload64;

	while (true) {
		switch (par->stage) {
			case VTM_WS_PARSE_MSG_BEGIN:
				par->msg_buf_used = 0;
				par->msg_frame_count = 0;
				par->msg_type = VTM_WS_MSG_CLOSE;
				par->stage = VTM_WS_PARSE_FRAME_BEGIN;
				break;

			case VTM_WS_PARSE_FRAME_BEGIN:
				par->payload_begin = 0;
				par->payload_len = 0;
				par->stage = VTM_WS_PARSE_FRAME_FIN_OPCODE;
				break;

			case VTM_WS_PARSE_FRAME_FIN_OPCODE:
				if (!VTM_BUF_GET_AVAIL(buf,1)) {
					stat = VTM_NET_RECV_STAT_INVALID;
					goto end;
				}
				c = VTM_BUF_GETC(buf);
				par->fin = c >> 7;
				par->rsv1 = c >> 6 & 0x01;
				par->rsv2 = c >> 5 & 0x01;
				par->rsv3 = c >> 4 & 0x01;
				par->opcode = c & 0x0f;

				/* rsv1-3 must be zero */
				if (par->rsv1 || par->rsv2 || par->rsv3) {
					vtm_err_set(VTM_E_WS_RSV_NONZERO);
					goto invalid;
				}

				/* opcode must be valid */
				if (!vtm_ws_parser_is_valid_opcode(par->opcode)) {
					vtm_err_set(VTM_E_WS_INVALID_OPCODE);
					goto invalid;
				}

				/* control messages must not be fragmented */
				if (par->opcode >= VTM_WS_OPCODE_CLOSE && !par->fin) {
					vtm_err_set(VTM_E_WS_CTRL_MSG_FRAGMENTED);
					goto invalid;
				}

				/* continuation frame must have predecessor */
				if (par->opcode == VTM_WS_OPCODE_CONTINUE && par->msg_frame_count < 1) {
					vtm_err_set(VTM_E_WS_BAD_CONTINUE_FRAME);
					goto invalid;
				}

				/* existing frames must be continued */
				if (par->msg_frame_count > 0
						&& par->opcode > VTM_WS_OPCODE_CONTINUE
						&& par->opcode < VTM_WS_OPCODE_CLOSE) {
					vtm_err_set(VTM_E_WS_MSG_NOT_CONTINUED);
					goto invalid;
				}

				/* store msg type if beginning of non ctrl frame */
				if (par->msg_frame_count == 0 && par->opcode < VTM_WS_OPCODE_CLOSE)
					par->msg_type = vtm_ws_parser_convert_opcode(par->opcode);

				par->stage = VTM_WS_PARSE_FRAME_MASK_LEN7;
				break;

			case VTM_WS_PARSE_FRAME_MASK_LEN7:
				if (!VTM_BUF_GET_AVAIL(buf, 1)) {
					stat = VTM_NET_RECV_STAT_AGAIN;
					goto end;
				}

				c = VTM_BUF_GETC(buf);
				par->masked = c >> 7;
				par->payload_len = c & 0x7f;

				/* client message must be masked */
				if (par->mode == VTM_WS_MODE_SERVER && !par->masked) {
					vtm_err_set(VTM_E_WS_CLIENT_MSG_UNMASKED);
					goto invalid;
				}

				/* server message must not be masked */
				if (par->mode == VTM_WS_MODE_CLIENT && par->masked) {
					vtm_err_set(VTM_E_WS_SERVER_MSG_MASKED);
					goto invalid;
				}

				if (par->payload_len == VTM_WS_LEN16_ID)
					par->stage = VTM_WS_PARSE_FRAME_LEN16;
				else if ( par->payload_len == VTM_WS_LEN64_ID)
					par->stage = VTM_WS_PARSE_FRAME_LEN64;
				else
					par->stage = VTM_WS_PARSE_FRAME_MASK32;
				break;

			case VTM_WS_PARSE_FRAME_LEN16:
				if (!VTM_BUF_GET_AVAIL(buf, 2)) {
					stat = VTM_NET_RECV_STAT_AGAIN;
					goto end;
				}

				par->payload_len  = ((uint16_t) VTM_BUF_GETC(buf))  << 8;
				par->payload_len += ((uint16_t) VTM_BUF_GETC(buf));

				/* 16bit length must be greater than 7bit max len */
				if (par->payload_len <= VTM_WS_LEN7_MAX) {
					vtm_err_set(VTM_E_WS_INVALID_PAYLOAD_LEN);
					goto invalid;
				}

				par->stage = VTM_WS_PARSE_FRAME_MASK32;
				break;

			case VTM_WS_PARSE_FRAME_LEN64:
				if (!VTM_BUF_GET_AVAIL(buf, 8)) {
					stat = VTM_NET_RECV_STAT_AGAIN;
					goto end;
				}

				payload64  = ((uint64_t) VTM_BUF_GETC(buf)) << 56;
				payload64 += ((uint64_t) VTM_BUF_GETC(buf)) << 48;
				payload64 += ((uint64_t) VTM_BUF_GETC(buf)) << 40;
				payload64 += ((uint64_t) VTM_BUF_GETC(buf)) << 32;
				payload64 += ((uint64_t) VTM_BUF_GETC(buf)) << 24;
				payload64 += ((uint64_t) VTM_BUF_GETC(buf)) << 16;
				payload64 += ((uint64_t) VTM_BUF_GETC(buf)) << 8;
				payload64 += ((uint64_t) VTM_BUF_GETC(buf));

				/* msb of 64bit payload length must be 0 */
				if (payload64 >> 31 & 0x01) {
					vtm_err_set(VTM_E_WS_INVALID_PAYLOAD_LEN);
					goto invalid;
				}

				/* 64bit length must be greater than 16bit max len */
				if (payload64 <= VTM_WS_LEN16_MAX) {
					vtm_err_set(VTM_E_WS_INVALID_PAYLOAD_LEN);
					goto invalid;
				}

				/* payload cannot be greater than size_t maximum */
				if (payload64 > SIZE_MAX) {
					vtm_err_set(VTM_E_WS_INVALID_PAYLOAD_LEN);
					goto invalid;
				}

				par->payload_len = (size_t) payload64;
				par->stage = VTM_WS_PARSE_FRAME_MASK32;
				break;

			case VTM_WS_PARSE_FRAME_MASK32:
				if (par->masked) {
					if (!VTM_BUF_GET_AVAIL(buf, 4)) {
						stat = VTM_NET_RECV_STAT_AGAIN;
						goto end;
					}

					par->mask = 0;
					for (i=0; i < 4; i++)
						par->mask += ((uint32_t) VTM_BUF_GETC(buf)) << (24-8*i);
				}

				par->payload_begin = buf->read;
				par->stage = VTM_WS_PARSE_FRAME_PAYLOAD;
				break;


			case VTM_WS_PARSE_FRAME_PAYLOAD:
				if (!VTM_BUF_GET_AVAIL(buf, par->payload_len)) {
					stat = VTM_NET_RECV_STAT_AGAIN;
					goto end;
				}

				rc = vtm_buf_mark_processed(buf, par->payload_len);
				if (rc != VTM_OK) {
					/* overflow */
					stat = VTM_NET_RECV_STAT_ERROR;
					goto end;
				}

				/* unmask payload */
				if (par->masked) {
					vtm_ws_frame_mask_payload(buf->data + par->payload_begin,
						par->payload_len, par->mask);
				}

				par->stage = vtm_ws_parser_is_control_opcode(par->opcode) ?
					VTM_WS_PARSE_FRAME_FINISH_CTRL :
					VTM_WS_PARSE_FRAME_FINISH_DATA;
				break;

			case VTM_WS_PARSE_FRAME_FINISH_CTRL:
				/* there should be no previous ctrl message */
				if (par->has_ctrl_msg) {
					vtm_err_sets(VTM_ERROR, "Has previous ctrl message");
					goto invalid;
				}

				/* allocate buffer for ctrl payload */
				payload = malloc(par->payload_len);
				if (!payload) {
					vtm_err_oom();
					stat = VTM_NET_RECV_STAT_ERROR;
					goto end;
				}
				memcpy(payload, buf->data + par->payload_begin, par->payload_len);

				/* create ctrl message */
				vtm_ws_msg_init(&par->ctrl_msg,
					vtm_ws_parser_convert_opcode(par->opcode),
					payload, par->payload_len);

				par->has_ctrl_msg = true;
				par->stage = VTM_WS_PARSE_FRAME_COMPLETE;
				break;

			case VTM_WS_PARSE_FRAME_FINISH_DATA:
				if (par->msg_buf_size - par->msg_buf_used < par->payload_len) {
					if (par->msg_buf_size < VTM_WS_PARSER_MSG_BUF_MAX) {
						par->msg_buf_size *= 2;
						par->msg_buf = realloc(par->msg_buf, par->msg_buf_size);
						if (!par->msg_buf) {
							vtm_err_oom();
							stat = VTM_NET_RECV_STAT_ERROR;
							goto end;
						}
					}
					else {
						stat = VTM_NET_RECV_STAT_ERROR;
						goto end;
					}
				}

				memcpy(par->msg_buf + par->msg_buf_used,
					buf->data + par->payload_begin,
					par->payload_len);

				par->msg_buf_used += par->payload_len;
				par->msg_frame_count++;
				par->stage = VTM_WS_PARSE_FRAME_COMPLETE;
				break;

			case VTM_WS_PARSE_FRAME_COMPLETE:
				vtm_buf_discard_processed(buf);

				if (par->has_ctrl_msg) {
					par->stage = VTM_WS_PARSE_FRAME_BEGIN;
					stat = VTM_NET_RECV_STAT_COMPLETE;
					goto end;
				}

				par->stage = (par->fin) ? VTM_WS_PARSE_MSG_COMPLETE : VTM_WS_PARSE_FRAME_BEGIN;
				break;

			case VTM_WS_PARSE_MSG_COMPLETE:
				par->stage = VTM_WS_PARSE_MSG_BEGIN;
				stat = VTM_NET_RECV_STAT_COMPLETE;
				goto end;

			case VTM_WS_PARSE_ERROR:
				stat = VTM_NET_RECV_STAT_ERROR;
				goto end;

			default:
				stat = VTM_NET_RECV_STAT_INVALID;
				goto end;
		}

	}

end:
	return stat;

invalid:
	par->stage = VTM_WS_PARSE_ERROR;
	return VTM_NET_RECV_STAT_INVALID;
}

static bool vtm_ws_parser_is_valid_opcode(unsigned int opcode)
{
	switch (opcode) {
		case VTM_WS_OPCODE_CONTINUE:
		case VTM_WS_OPCODE_TEXT:
		case VTM_WS_OPCODE_BINARY:
		case VTM_WS_OPCODE_CLOSE:
		case VTM_WS_OPCODE_PING:
		case VTM_WS_OPCODE_PONG:
			return true;
	}

	return false;
}

static bool vtm_ws_parser_is_control_opcode(unsigned int opcode)
{
	switch (opcode) {
		case VTM_WS_OPCODE_CLOSE:
		case VTM_WS_OPCODE_PING:
		case VTM_WS_OPCODE_PONG:
			return true;
	}

	return false;
}

static enum vtm_ws_msg_type vtm_ws_parser_convert_opcode(unsigned int opcode)
{
	switch (opcode) {
		case VTM_WS_MSG_TEXT:
		case VTM_WS_MSG_BINARY:
		case VTM_WS_MSG_CLOSE:
		case VTM_WS_MSG_PING:
		case VTM_WS_MSG_PONG:
			return opcode;

		default:
			VTM_ABORT_NOT_SUPPORTED;
	}

	VTM_ABORT_NOT_REACHABLE;
	return VTM_WS_MSG_CLOSE;
}

int vtm_ws_parser_get_msg(struct vtm_ws_parser *par, struct vtm_ws_msg *msg)
{
	/* interleaved ctrl message available */
	if (par->has_ctrl_msg) {
		par->has_ctrl_msg = false;
		*msg = par->ctrl_msg;
		return VTM_OK;
	}

	vtm_ws_msg_init(msg, par->msg_type, par->msg_buf, par->msg_buf_used);

	par->msg_buf = malloc(par->msg_buf_size);
	if (!par->msg_buf) {
		vtm_err_oom();
		return vtm_err_get_code();
	}

	return VTM_OK;
}
