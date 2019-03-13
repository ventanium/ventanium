/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "buffer.h"

#include <stdlib.h> /* free() */
#include <string.h> /* strlen() */
#include <vtm/core/error.h>
#include <vtm/core/math.h>

/* forward declaration */
static void vtm_buf_grow(struct vtm_buf *buf, size_t add_size);

struct vtm_buf* vtm_buf_new(enum vtm_byteorder order)
{
	struct vtm_buf *buf;

	buf = malloc(sizeof(*buf));
	if (!buf) {
		vtm_err_oom();
		return NULL;
	}

	vtm_buf_init(buf, order);

	return buf;
}

void vtm_buf_init(struct vtm_buf *buf, enum vtm_byteorder order)
{
	buf->order = order;
	buf->data = buf->sdata;
	buf->len = sizeof(buf->sdata);
	buf->used = 0;
	buf->read = 0;
	buf->err = VTM_OK;
}

void vtm_buf_release(struct vtm_buf *buf)
{
	if (buf->data != buf->sdata)
		free(buf->data);
}

void vtm_buf_free(struct vtm_buf *buf)
{
	vtm_buf_release(buf);
	free(buf);
}

void vtm_buf_clear(struct vtm_buf *buf)
{
	buf->used = 0;
	buf->read = 0;
	buf->err = VTM_OK;
}

int vtm_buf_ensure(struct vtm_buf *buf, size_t space)
{
	if(!VTM_BUF_PUT_AVAIL(buf, space))
		vtm_buf_grow(buf, space);

	return buf->err;
}

void vtm_buf_discard_processed(struct vtm_buf *buf)
{
	if (buf->read == 0)
		return;

	memmove(buf->data, buf->data + buf->read, buf->used - buf->read);
	buf->used -= buf->read;
	buf->read = 0;
}

int vtm_buf_mark_processed(struct vtm_buf *buf, size_t num)
{
	return vtm_math_size_add(buf->read, num, &buf->read);
}

int vtm_buf_geto(struct vtm_buf *buf, void *dst, size_t len, enum vtm_byteorder dst_order)
{
	size_t i;

	if (buf->order == dst_order) {
		memcpy(dst, buf->data + buf->read, len);
	}
	else {
		for (i=0; i < len; i++)
			((unsigned char*) dst)[len-i-1] = (buf->data + buf->read)[i];
	}

	buf->read += len;

	return VTM_OK;
}

int vtm_buf_getm(struct vtm_buf *buf, void *dst, size_t len)
{
	return vtm_buf_geto(buf, dst, len, buf->order);
}

int vtm_buf_puto(struct vtm_buf *buf, const void *src, size_t len, enum vtm_byteorder src_order)
{
	size_t i;

	if (!VTM_BUF_PUT_AVAIL(buf, len))
		vtm_buf_grow(buf, len);

	if (buf->err != VTM_OK)
		return buf->err;

	if (buf->order == src_order) {
		memcpy(buf->data + buf->used, src, len);
		buf->used += len;
	}
	else {
		for (i=len; i > 0; i--)
			buf->data[buf->used++] = ((unsigned char*) src)[i-1];
	}

	return VTM_OK;
}

int vtm_buf_putm(struct vtm_buf *buf, const void *src, size_t len)
{
	return vtm_buf_puto(buf, src, len, buf->order);
}

int vtm_buf_puts(struct vtm_buf *buf, const char *str)
{
	return vtm_buf_putm(buf, str, strlen(str));
}

int vtm_buf_putc(struct vtm_buf *buf, unsigned char c)
{
	if (!VTM_BUF_PUT_AVAIL(buf, 1))
		vtm_buf_grow(buf, 1);

	if (buf->err != VTM_OK)
		return buf->err;

	buf->data[buf->used++] = c;

	return VTM_OK;
}

static void vtm_buf_grow(struct vtm_buf *buf, size_t add_size)
{
	unsigned char *mem;
	size_t old, target;

	if (buf->err != VTM_OK)
		return;

	old = buf->len;

	/* calculate target */
	target = old + add_size;
	if (target < old)
		goto err_overflow;

	/* find matching buffer size */
	while (buf->len < target) {
		buf->len = buf->len << 1;
		if (buf->len < old)
			goto err_overflow;
	}

	if (buf->data == buf->sdata) {
		buf->data = malloc(buf->len);
		if (!buf->data)
			goto err_oom;
		memcpy(buf->data, buf->sdata, sizeof(buf->sdata));
	}
	else {
		mem = realloc(buf->data, buf->len);
		if (!mem)
			goto err_oom;
		buf->data = mem;
	}
	return;

err_overflow:
	buf->len = old;
	buf->err = VTM_E_OVERFLOW;
	return;

err_oom:
	vtm_err_oom();
	buf->len = old;
	buf->err = VTM_E_MALLOC;
}
