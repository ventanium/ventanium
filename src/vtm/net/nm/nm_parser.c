/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "nm_parser_intl.h"

#include <stdlib.h> /* malloc(), free() */
#include <vtm/core/blob.h>
#include <vtm/core/error.h>
#include <vtm/core/types.h>
#include <vtm/core/variant.h>
#include <vtm/net/nm/nm_protocol_intl.h>
#include <vtm/util/serialization.h>

void vtm_nm_parser_init(struct vtm_nm_parser *par)
{
	par->order = vtm_sys_get_byteorder();
	par->state = VTM_NM_PARSE_MSG_BEGIN;
	par->msg = NULL;
	par->name = NULL;
}

void vtm_nm_parser_release(struct vtm_nm_parser *par)
{
	if (par->msg)
		vtm_dataset_free(par->msg);

	if (par->name)
		free(par->name);
}

void vtm_nm_parser_reset(struct vtm_nm_parser *par)
{
	vtm_nm_parser_release(par);
	vtm_nm_parser_init(par);
}

enum vtm_net_recv_stat vtm_nm_parser_run(struct vtm_nm_parser *par, struct vtm_buf *buf)
{
	int rc;
	char c;
	struct vtm_variant var;
	uint32_t var32;
	uint64_t var64;

	while (true) {
		switch (par->state) {
			case VTM_NM_PARSE_MSG_BEGIN:
				par->state = VTM_NM_PARSE_MAGIC;
				break;

			case VTM_NM_PARSE_MAGIC:
				if (!VTM_BUF_GET_AVAIL(buf, 1))
					return VTM_NET_RECV_STAT_AGAIN;

				c = VTM_BUF_GETC(buf);
				if (c != VTM_NM_PROTO_MAGIC)
					return VTM_NET_RECV_STAT_INVALID;

				par->state = VTM_NM_PARSE_VERSION;
				break;

			case VTM_NM_PARSE_VERSION:
				if (!VTM_BUF_GET_AVAIL(buf, 1))
					return VTM_NET_RECV_STAT_AGAIN;

				c = VTM_BUF_GETC(buf);
				if (c != VTM_NM_PROTO_VER_1)
					return VTM_NET_RECV_STAT_INVALID;

				par->state = VTM_NM_PARSE_FIELD_COUNT;
				break;

			case VTM_NM_PARSE_FIELD_COUNT:
				if (!VTM_BUF_GET_AVAIL(buf, 2))
					return VTM_NET_RECV_STAT_AGAIN;

				vtm_buf_geto(buf, &par->field_count, sizeof(par->field_count), par->order);
				par->fields_parsed = 0;
				par->state = VTM_NM_PARSE_FIELD_BEGIN;
				break;

			case VTM_NM_PARSE_FIELD_BEGIN:
				if (par->fields_parsed == par->field_count)
					par->state = VTM_NM_PARSE_MSG_COMPLETE;
				else
					par->state = VTM_NM_PARSE_NAME_LEN;
				break;

			case VTM_NM_PARSE_NAME_LEN:
				if (!VTM_BUF_GET_AVAIL(buf, 1))
					return VTM_NET_RECV_STAT_AGAIN;

				vtm_buf_geto(buf, &par->name_len, sizeof(par->name_len), par->order);
				if (par->name_len == 0)
					return VTM_NET_RECV_STAT_INVALID;

				par->state = VTM_NM_PARSE_NAME;
				break;

			case VTM_NM_PARSE_NAME:
				if (!VTM_BUF_GET_AVAIL(buf, par->name_len))
					return VTM_NET_RECV_STAT_AGAIN;

				par->name = malloc(par->name_len+1);
				if (!par->name) {
					vtm_err_oom();
					return VTM_NET_RECV_STAT_ERROR;
				}

				vtm_buf_getm(buf, par->name, par->name_len);
				par->name[par->name_len] = '\0';
				par->state = VTM_NM_PARSE_VALUE_TYPE;
				break;

			case VTM_NM_PARSE_VALUE_TYPE:
				if (!VTM_BUF_GET_AVAIL(buf, 1))
					return VTM_NET_RECV_STAT_AGAIN;

				c = VTM_BUF_GETC(buf);
				rc = vtm_nm_type_from_num(&par->value_type, c);
				if (rc != VTM_OK)
					return VTM_NET_RECV_STAT_INVALID;

				switch (par->value_type) {
					case VTM_ELEM_FLOAT:
						par->value_len = sizeof(uint32_t);
						par->state = VTM_NM_PARSE_VALUE;
						break;

					case VTM_ELEM_DOUBLE:
						par->value_len = sizeof(uint64_t);
						par->state = VTM_NM_PARSE_VALUE;
						break;

					case VTM_ELEM_STRING:
					case VTM_ELEM_BLOB:
						par->state = VTM_NM_PARSE_VALUE_LEN;
						break;

					default:
						par->value_len = (uint32_t) vtm_elem_size(par->value_type);
						par->state = VTM_NM_PARSE_VALUE;
						break;
				}
				break;

			case VTM_NM_PARSE_VALUE_LEN:
				if (!VTM_BUF_GET_AVAIL(buf, 4))
					return VTM_NET_RECV_STAT_AGAIN;

				vtm_buf_geto(buf, &par->value_len, sizeof(par->value_len), par->order);
				par->state = VTM_NM_PARSE_VALUE;
				break;

			case VTM_NM_PARSE_VALUE:
				if (!VTM_BUF_GET_AVAIL(buf, par->value_len))
					return VTM_NET_RECV_STAT_AGAIN;

				switch (par->value_type) {
					case VTM_ELEM_FLOAT:
						vtm_buf_geto(buf, &var32, sizeof(uint32_t), par->order);
						rc = vtm_unpack_float(var32, &par->value.elem_float);
						if (rc != VTM_OK)
							return VTM_NET_RECV_STAT_ERROR;
						break;

					case VTM_ELEM_DOUBLE:
						vtm_buf_geto(buf, &var64, sizeof(uint64_t), par->order);
						rc = vtm_unpack_double(var64, &par->value.elem_double);
						if (rc != VTM_OK)
							return VTM_NET_RECV_STAT_ERROR;
						break;

					case VTM_ELEM_STRING:
						par->value.elem_pointer = malloc(par->value_len+1);
						if (!par->value.elem_pointer) {
							vtm_err_oom();
							return VTM_NET_RECV_STAT_ERROR;
						}
						vtm_buf_getm(buf, par->value.elem_pointer, par->value_len);
						((char*) par->value.elem_pointer)[par->value_len] = '\0';
						break;

					case VTM_ELEM_BLOB:
						par->value.elem_pointer = vtm_blob_new(par->value_len);
						if (!par->value.elem_pointer)
							return VTM_NET_RECV_STAT_ERROR;
						vtm_buf_getm(buf, par->value.elem_pointer, par->value_len);
						break;

					default:
						vtm_buf_geto(buf, &par->value, par->value_len, par->order);
						break;
				}

				par->state = VTM_NM_PARSE_FIELD_COMPLETE;
				break;

			case VTM_NM_PARSE_FIELD_COMPLETE:
				if (!par->msg) {
					par->msg = vtm_dataset_new();
					if (!par->msg)
						return VTM_NET_RECV_STAT_ERROR;
				}

				var.type = par->value_type;
				var.data = par->value;

				vtm_dataset_set_variant(par->msg, par->name, &var);

				free(par->name);
				par->name = NULL;

				if (par->value_type == VTM_ELEM_STRING)
					free(par->value.elem_pointer);

				par->fields_parsed++;
				par->state = VTM_NM_PARSE_FIELD_BEGIN;
				break;

			case VTM_NM_PARSE_MSG_COMPLETE:
				return VTM_NET_RECV_STAT_COMPLETE;
		}
	}

	VTM_ABORT_NOT_REACHABLE;
	return VTM_NET_RECV_STAT_ERROR;
}

vtm_dataset* vtm_nm_parser_get_msg(struct vtm_nm_parser *par)
{
	vtm_dataset *msg;

	if (par->state != VTM_NM_PARSE_MSG_COMPLETE) {
		vtm_err_set(VTM_E_INVALID_STATE);
		return NULL;
	}

	msg = par->msg;
	par->msg = NULL;
	par->state = VTM_NM_PARSE_MSG_BEGIN;

	return msg;
}
