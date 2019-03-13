/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "nm_protocol_intl.h"

#include <stdlib.h> /* free() */
#include <string.h> /* strlen() */
#include <vtm/core/error.h>
#include <vtm/core/format.h>
#include <vtm/net/common.h>
#include <vtm/net/nm/nm_parser_intl.h>
#include <vtm/util/serialization.h>

int vtm_nm_type_to_num(enum vtm_elem_type type, unsigned char *c)
{
	switch (type) {
		case VTM_ELEM_INT8:      *c = VTM_NM_TNUM_INT8;    return VTM_OK;
		case VTM_ELEM_UINT8:     *c = VTM_NM_TNUM_UINT8;   return VTM_OK;
		case VTM_ELEM_INT16:     *c = VTM_NM_TNUM_INT16;   return VTM_OK;
		case VTM_ELEM_UINT16:    *c = VTM_NM_TNUM_UINT16;  return VTM_OK;
		case VTM_ELEM_INT32:     *c = VTM_NM_TNUM_INT32;   return VTM_OK;
		case VTM_ELEM_UINT32:    *c = VTM_NM_TNUM_UINT32;  return VTM_OK;
		case VTM_ELEM_INT64:     *c = VTM_NM_TNUM_INT64;   return VTM_OK;
		case VTM_ELEM_UINT64:    *c = VTM_NM_TNUM_UINT64;  return VTM_OK;
		case VTM_ELEM_BOOL:      *c = VTM_NM_TNUM_BOOL;    return VTM_OK;
		case VTM_ELEM_CHAR:      *c = VTM_NM_TNUM_CHAR;    return VTM_OK;
		case VTM_ELEM_SCHAR:     *c = VTM_NM_TNUM_SCHAR;   return VTM_OK;
		case VTM_ELEM_UCHAR:     *c = VTM_NM_TNUM_UCHAR;   return VTM_OK;
		case VTM_ELEM_FLOAT:     *c = VTM_NM_TNUM_FLOAT;   return VTM_OK;
		case VTM_ELEM_DOUBLE:    *c = VTM_NM_TNUM_DOUBLE;  return VTM_OK;
		case VTM_ELEM_STRING:    *c = VTM_NM_TNUM_STRING;  return VTM_OK;

		default:
			break;
	}

	return VTM_E_NOT_SUPPORTED;
}

int vtm_nm_type_from_num(enum vtm_elem_type *type, unsigned char c)
{
	switch (c) {
		case VTM_NM_TNUM_INT8:   *type = VTM_ELEM_INT8;    return VTM_OK;
		case VTM_NM_TNUM_UINT8:  *type = VTM_ELEM_UINT8;   return VTM_OK;
		case VTM_NM_TNUM_INT16:  *type = VTM_ELEM_INT16;   return VTM_OK;
		case VTM_NM_TNUM_UINT16: *type = VTM_ELEM_UINT16;  return VTM_OK;
		case VTM_NM_TNUM_INT32:  *type = VTM_ELEM_INT32;   return VTM_OK;
		case VTM_NM_TNUM_UINT32: *type = VTM_ELEM_UINT32;  return VTM_OK;
		case VTM_NM_TNUM_INT64:  *type = VTM_ELEM_INT64;   return VTM_OK;
		case VTM_NM_TNUM_UINT64: *type = VTM_ELEM_UINT64;  return VTM_OK;
		case VTM_NM_TNUM_BOOL:   *type = VTM_ELEM_BOOL;    return VTM_OK;
		case VTM_NM_TNUM_CHAR:   *type = VTM_ELEM_CHAR;    return VTM_OK;
		case VTM_NM_TNUM_SCHAR:  *type = VTM_ELEM_SCHAR;   return VTM_OK;
		case VTM_NM_TNUM_UCHAR:  *type = VTM_ELEM_UCHAR;   return VTM_OK;
		case VTM_NM_TNUM_FLOAT:  *type = VTM_ELEM_FLOAT;   return VTM_OK;
		case VTM_NM_TNUM_DOUBLE: *type = VTM_ELEM_DOUBLE;  return VTM_OK;
		case VTM_NM_TNUM_STRING: *type = VTM_ELEM_STRING;  return VTM_OK;

		default:
			break;
	}

	return VTM_E_NOT_SUPPORTED;
}

int vtm_nm_msg_to_buf(vtm_dataset *msg, struct vtm_buf *buf)
{
	int rc;
	vtm_list *entries;
	struct vtm_dataset_entry *entry;
	size_t i, count, len;
	uint16_t var16;
	uint32_t var32;
	uint64_t var64;
	enum vtm_byteorder sys_order;
	unsigned char c;

	/* get system byteorder */
	sys_order = vtm_sys_get_byteorder();

	/* get message entries */
	entries = vtm_dataset_entryset(msg);
	if (!entries)
		return vtm_err_sets(VTM_ERROR, "No entries!");

	/* write magic and version */
	vtm_buf_putc(buf, VTM_NM_PROTO_MAGIC);
	vtm_buf_putc(buf, VTM_NM_PROTO_VER_1);

	/* write field count */
	count = vtm_list_size(entries);
	if (count > UINT16_MAX) {
		rc = vtm_err_set(VTM_E_OVERFLOW);
		goto end;
	}
	var16 = (uint16_t) count;
	vtm_buf_puto(buf, &var16, sizeof(uint16_t), sys_order);

	/* write fields */
	for (i=0; i < count; i++) {
		entry = vtm_list_get_pointer(entries, i);
		len = strlen(entry->name);
		if (len > UCHAR_MAX) {
			rc = vtm_err_setf(VTM_E_NOT_SUPPORTED, "Field name too long %s",
				entry->name);
			goto end;
		}

		/* write name length + name */
		c = (unsigned char) len;
		vtm_buf_putc(buf, c);
		vtm_buf_puts(buf, entry->name);

		/* write type */
		rc = vtm_nm_type_to_num(entry->var->type, &c);
		if (rc != VTM_OK)
			goto end;

		vtm_buf_putc(buf, c);

		/* write content */
		switch (entry->var->type) {
			case VTM_ELEM_FLOAT:
				rc = vtm_pack_float(entry->var->data.elem_float, &var32);
				if (rc != VTM_OK)
					goto end;
				vtm_buf_puto(buf, &var32, sizeof(uint32_t), sys_order);
				break;

			case VTM_ELEM_DOUBLE:
				rc = vtm_pack_double(entry->var->data.elem_double, &var64);
				if (rc != VTM_OK)
					goto end;
				vtm_buf_puto(buf, &var64, sizeof(uint64_t), sys_order);
				break;

			case VTM_ELEM_STRING:
				len = strlen((char*) entry->var->data.elem_pointer);
				if (len > UINT32_MAX) {
					rc = vtm_err_sets(VTM_E_NOT_SUPPORTED, "String too long");
					goto end;
				}

				var32 = (uint32_t) len;
				vtm_buf_puto(buf, &var32, sizeof(uint32_t), sys_order);
				vtm_buf_putm(buf, entry->var->data.elem_pointer, len);
				break;

			default:
				vtm_buf_puto(buf, &entry->var->data,
					vtm_elem_size(entry->var->type), sys_order);
				break;
		}

	}

	/* check buffer error */
	if (buf->err != VTM_OK)
		rc = buf->err;

end:
	/* free resources */
	vtm_list_free(entries);

	return rc;
}

int vtm_nm_msg_from_buf(vtm_dataset *msg, struct vtm_buf *buf)
{
	struct vtm_nm_parser par;
	enum vtm_net_recv_stat stat;

	vtm_nm_parser_init(&par);
	par.msg = msg;
	stat = vtm_nm_parser_run(&par, buf);
	par.msg = NULL;
	vtm_nm_parser_release(&par);

	return (stat == VTM_NET_RECV_STAT_COMPLETE) ? VTM_OK : VTM_ERROR;
}
