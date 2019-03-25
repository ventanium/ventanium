/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "mysql_bind_buffer_intl.h"

#include <stdlib.h> /* malloc() */
#include <string.h> /* strlen(), memcpy() */
#include <time.h>
#include <vtm/core/blob.h>
#include <vtm/core/error.h>
#include <vtm/core/string.h>

/* forward declaration */
static uint64_t vtm_mysql_bind_buf_convert_timestamp(MYSQL_TIME *mtime);

int vtm_mysql_bind_buf_init_from_field(struct vtm_mysql_bind_buf *buf, MYSQL_FIELD *field)
{
	bool use_static;

	buf->name = field->name;
	buf->type = field->type;
	buf->content_null = false;

	use_static = true;

	switch (field->type) {
		case MYSQL_TYPE_NULL:
			buf->content_length = 0;
			break;

		case MYSQL_TYPE_TINY:
			buf->content_length = sizeof(int8_t);
			break;

		case MYSQL_TYPE_SHORT:
			buf->content_length = sizeof(int16_t);
			break;

		case MYSQL_TYPE_LONG:
			buf->content_length = sizeof(int32_t);
			break;

		case MYSQL_TYPE_LONGLONG:
			buf->content_length = sizeof(int64_t);
			break;

		case MYSQL_TYPE_FLOAT:
			buf->content_length = sizeof(float);
			break;

		case MYSQL_TYPE_DOUBLE:
			buf->content_length = sizeof(double);
			break;

		case MYSQL_TYPE_DATE:
		case MYSQL_TYPE_DATETIME:
		case MYSQL_TYPE_TIME:
		case MYSQL_TYPE_TIMESTAMP:
			buf->content_length = sizeof(MYSQL_TIME);
			use_static = false;
			break;

		case MYSQL_TYPE_VAR_STRING:
			buf->content_length = field->length + 1;
			use_static = false;
			break;

		case MYSQL_TYPE_BLOB:
			buf->content_length = field->length;
			use_static = false;
			break;

		default:
			return VTM_E_NOT_SUPPORTED;
	}

	if (use_static) {
		buf->data = &buf->static_data;
	}
	else {
		buf->data = malloc(buf->content_length);
		if (!buf->data) {
			vtm_err_oom();
			return vtm_err_get_code();
		}
	}

	return VTM_OK;
}

int vtm_mysql_bind_buf_init_from_variant(struct vtm_mysql_bind_buf *buf, struct vtm_variant *var)
{
	size_t len;
	const char *str;
	const void *blob;
	bool use_static;

	vtm_mysql_bind_buf_release(buf);

	if (!var) {
		buf->type = MYSQL_TYPE_NULL;
		buf->content_null = true;
		buf->content_length = 0;
		buf->data = NULL;
		return VTM_OK;
	}

	use_static = true;
	switch (var->type) {
		case VTM_ELEM_NULL:
			buf->type = MYSQL_TYPE_NULL;
			buf->content_null = true;
			buf->content_length = 0;
			break;

		case VTM_ELEM_INT8:
		case VTM_ELEM_UINT8:
		case VTM_ELEM_BOOL:
		case VTM_ELEM_CHAR:
		case VTM_ELEM_SCHAR:
		case VTM_ELEM_UCHAR:
			buf->type = MYSQL_TYPE_TINY;
			buf->static_data.elem_int8 = vtm_variant_as_int8(var);
			buf->content_length = sizeof(int8_t);
			break;

		case VTM_ELEM_INT16:
		case VTM_ELEM_UINT16:
		case VTM_ELEM_SHORT:
		case VTM_ELEM_USHORT:
			buf->type = MYSQL_TYPE_SHORT;
			buf->static_data.elem_int16 = vtm_variant_as_int16(var);
			buf->content_length = sizeof(int16_t);
			break;

		case VTM_ELEM_INT32:
		case VTM_ELEM_UINT32:
		case VTM_ELEM_INT:
		case VTM_ELEM_UINT:
			buf->type = MYSQL_TYPE_LONG;
			buf->static_data.elem_int32 = vtm_variant_as_int32(var);
			buf->content_length = sizeof(int32_t);
			break;

		case VTM_ELEM_INT64:
		case VTM_ELEM_UINT64:
		case VTM_ELEM_LONG:
		case VTM_ELEM_ULONG:
			buf->type = MYSQL_TYPE_LONGLONG;
			buf->static_data.elem_int64 = vtm_variant_as_int64(var);
			buf->content_length = sizeof(int64_t);
			break;

		case VTM_ELEM_FLOAT:
			buf->type = MYSQL_TYPE_FLOAT;
			buf->static_data.elem_float = vtm_variant_as_float(var);
			buf->content_length = sizeof(float);
			break;

		case VTM_ELEM_DOUBLE:
			buf->type = MYSQL_TYPE_DOUBLE;
			buf->static_data.elem_double = vtm_variant_as_double(var);
			buf->content_length = sizeof(double);
			break;

		case VTM_ELEM_STRING:
			buf->type = MYSQL_TYPE_VAR_STRING;
			str = vtm_variant_as_str(var);
			len = strlen(str);
			buf->data = malloc(len);
			if (!buf->data) {
				vtm_err_oom();
				return vtm_err_get_code();
			}
			memcpy(buf->data, str, len);
			buf->content_length = len;
			use_static = false;
			break;

		case VTM_ELEM_BLOB:
			buf->type = MYSQL_TYPE_BLOB;
			blob = vtm_variant_as_blob(var);
			len = vtm_blob_size(blob);
			buf->data = malloc(len);
			if (!buf->data) {
				vtm_err_oom();
				return vtm_err_get_code();
			}
			memcpy(buf->data, blob, len);
			buf->content_length = len;
			use_static = false;
			break;

		default:
			return VTM_E_NOT_SUPPORTED;
	}

	if (use_static)
		buf->data = &buf->static_data;

	return VTM_OK;
}

void vtm_mysql_bind_buf_release(struct vtm_mysql_bind_buf *buf)
{
	if (buf->data != NULL && buf->data != &buf->static_data)
		free(buf->data);

	buf->data = NULL;
}

int vtm_mysql_bind_buf_read(struct vtm_mysql_bind_buf *buf, vtm_dataset *ds)
{
	void *blob;

	VTM_ASSERT(buf->name != NULL);

	if (buf->content_null)
		return VTM_OK;

	switch (buf->type) {
		case MYSQL_TYPE_TINY:
			vtm_dataset_set_int8(ds, buf->name, buf->static_data.elem_int8);
			break;

		case MYSQL_TYPE_SHORT:
			vtm_dataset_set_int16(ds, buf->name, buf->static_data.elem_int16);
			break;

		case MYSQL_TYPE_LONG:
			vtm_dataset_set_int32(ds, buf->name, buf->static_data.elem_int32);
			break;

		case MYSQL_TYPE_LONGLONG:
			vtm_dataset_set_int64(ds, buf->name, buf->static_data.elem_int64);
			break;

		case MYSQL_TYPE_FLOAT:
			vtm_dataset_set_float(ds, buf->name, buf->static_data.elem_float);
			break;

		case MYSQL_TYPE_DOUBLE:
			vtm_dataset_set_double(ds, buf->name, buf->static_data.elem_double);
			break;

		case MYSQL_TYPE_DATE:
		case MYSQL_TYPE_DATETIME:
		case MYSQL_TYPE_TIME:
		case MYSQL_TYPE_TIMESTAMP:
			vtm_dataset_set_uint64(ds, buf->name,
				vtm_mysql_bind_buf_convert_timestamp((MYSQL_TIME*) buf->data));
			break;

		case MYSQL_TYPE_VAR_STRING:
			vtm_dataset_set_string(ds, buf->name, (char*) buf->data);
			break;

		case MYSQL_TYPE_BLOB:
			blob = vtm_blob_new(buf->content_length);
			if (!blob)
				return vtm_err_get_code();
			memcpy(blob, buf->data, buf->content_length);
			vtm_dataset_set_blob(ds, buf->name, blob);
			break;

		default:
			return vtm_err_set(VTM_E_NOT_SUPPORTED);
	}

	return VTM_OK;
}

void vtm_mysql_bind_buf_couple(struct vtm_mysql_bind_buf *buf, MYSQL_BIND *my_bind)
{
	my_bind->buffer_type = buf->type;
	my_bind->buffer_length = buf->content_length;
	my_bind->buffer = buf->data;
	my_bind->length = &(buf->content_length);
	my_bind->is_null = &(buf->content_null);
}

static uint64_t vtm_mysql_bind_buf_convert_timestamp(MYSQL_TIME *mtime)
{
	time_t rawtime;
	time_t ts;
	struct tm *conv;

	time(&rawtime);

	conv = localtime(&rawtime);
	conv->tm_year = mtime->year - 1900;
	conv->tm_mon = mtime->month - 1;
	conv->tm_mday = mtime->day;
	conv->tm_hour = mtime->hour;
	conv->tm_min = mtime->minute;
	conv->tm_sec = mtime->second;

	ts = mktime(conv);
	return (uint64_t) ((ts * 1000) + (mtime->second_part / 1000));
}
