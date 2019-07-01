/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "socket_emitter.h"

#include <string.h> /* memmove() */
#include <vtm/core/error.h>
#include <vtm/fs/file.h>

#define VTM_EMT_FILE_BUF_SIZE       4096

struct vtm_emt_raw
{
	struct vtm_socket_emitter se;
	const unsigned char *src;
	size_t buf_pos;
};

struct vtm_emt_buf
{
	struct vtm_emt_raw re;
	struct vtm_buf *buf;
};

struct vtm_emt_file
{
	struct vtm_socket_emitter se;
	char buf[VTM_EMT_FILE_BUF_SIZE];
	size_t buf_used;
	size_t buf_written;
	FILE *fp;
};

/* forward declaration */
static enum vtm_socket_emitter_result vtm_socket_emitter_write_raw(struct vtm_socket_emitter *se);
static enum vtm_socket_emitter_result vtm_socket_emitter_write_file(struct vtm_socket_emitter *se);
static void vtm_socket_emitter_clean_buf(struct vtm_socket_emitter *se);
static void vtm_socket_emitter_clean_file(struct vtm_socket_emitter *se);

void vtm_socket_emitter_free_chain(struct vtm_socket_emitter *se)
{
	struct vtm_socket_emitter *next;

	while (se) {
		next = se->next;
		vtm_socket_emitter_free_single(se);
		se = next;
	}
}

void vtm_socket_emitter_free_single(struct vtm_socket_emitter *se)
{
	if (se->vtm_sock_emt_clean)
		se->vtm_sock_emt_clean(se);

	free(se);
}

struct vtm_socket_emitter* vtm_socket_emitter_for_raw(vtm_socket *sock, const void *src, size_t len)
{
	struct vtm_emt_raw *re;

	if (len > UINT64_MAX) {
		vtm_err_set(VTM_E_INVALID_ARG);
		return NULL;
	}

	re = malloc(sizeof(*re));
	if (!re) {
		vtm_err_oom();
		return NULL;
	}

	re->src = src;
	re->buf_pos = 0;

	re->se.sock = sock;
	re->se.next = NULL;
	re->se.length = len;
	re->se.vtm_sock_emt_write = vtm_socket_emitter_write_raw;
	re->se.vtm_sock_emt_clean = NULL;

	return (struct vtm_socket_emitter*) re;
}

struct vtm_socket_emitter* vtm_socket_emitter_for_buffer(vtm_socket *sock, struct vtm_buf *buf, bool fr)
{
	struct vtm_emt_buf *be;

	be = malloc(sizeof(*be));
	if (!be) {
		vtm_err_oom();
		return NULL;
	}

	be->buf = buf;

	be->re.src = buf->data;
	be->re.buf_pos = 0;

	be->re.se.sock = sock;
	be->re.se.next = NULL;
	be->re.se.length = buf->used;
	be->re.se.vtm_sock_emt_write = vtm_socket_emitter_write_raw;
	be->re.se.vtm_sock_emt_clean = fr ? vtm_socket_emitter_clean_buf : NULL;

	return (struct vtm_socket_emitter*) be;
}

struct vtm_socket_emitter* vtm_socket_emitter_for_file(vtm_socket *sock, FILE *fp)
{
	struct vtm_emt_file *fe;

	fe = malloc(sizeof(*fe));
	if (!fe) {
		vtm_err_oom();
		return NULL;
	}

	fe->fp = fp;
	fe->buf_used = 0;
	fe->buf_written = 0;

	fe->se.sock = sock;
	fe->se.next = NULL;
	fe->se.length = vtm_file_get_fsize(fp);
	fe->se.vtm_sock_emt_write = vtm_socket_emitter_write_file;
	fe->se.vtm_sock_emt_clean = vtm_socket_emitter_clean_file;

	return (struct vtm_socket_emitter*) fe;
}

static enum vtm_socket_emitter_result vtm_socket_emitter_write_raw(struct vtm_socket_emitter *se)
{
	int rc;
	size_t written;
	struct vtm_emt_raw *re;

	re = (struct vtm_emt_raw*) se;

	rc = vtm_socket_write(re->se.sock, re->src + re->buf_pos,
	                      (size_t) (re->se.length - re->buf_pos), &written);

	switch (rc) {
		case VTM_E_IO_AGAIN:
			re->buf_pos += written;
			return VTM_SOCK_EMIT_AGAIN;

		case VTM_OK:
			if (re->buf_pos + written == re->se.length)
				return VTM_SOCK_EMIT_COMPLETE;
			break;

		default:
			break;
	}

	return VTM_SOCK_EMIT_ERROR;
}

static enum vtm_socket_emitter_result vtm_socket_emitter_write_file(struct vtm_socket_emitter *se)
{
	int rc;
	struct vtm_emt_file *fe;
	FILE *fp;
	size_t rcount;
	size_t wcount;
	size_t diff;

	fe = (struct vtm_emt_file*) se;
	fp = fe->fp;

	while (true) {
		rcount = fread(fe->buf + fe->buf_used, 1, VTM_EMT_FILE_BUF_SIZE - fe->buf_used, fp);
		if (rcount <= 0) {
			if (feof(fp) == 0)
				return VTM_SOCK_EMIT_ERROR;
			else
				return VTM_SOCK_EMIT_COMPLETE;
		}

		fe->buf_used += rcount;

		rc = vtm_socket_write(se->sock,
			fe->buf + fe->buf_written,
			fe->buf_used - fe->buf_written,
			&wcount);

		if (!(rc == VTM_OK || rc == VTM_E_IO_AGAIN))
			return VTM_SOCK_EMIT_ERROR;

		diff = VTM_EMT_FILE_BUF_SIZE - (fe->buf_written + wcount);
		if (diff > 0)
			memmove(fe->buf, fe->buf + fe->buf_written + wcount, diff);

		fe->buf_used = diff;
		fe->buf_written = 0;

		if (rc == VTM_E_IO_AGAIN)
			return VTM_SOCK_EMIT_AGAIN;
	}

	return VTM_SOCK_EMIT_ERROR;
}

static void vtm_socket_emitter_clean_buf(struct vtm_socket_emitter *se)
{
	vtm_buf_free(((struct vtm_emt_buf*) se)->buf);
}

static void vtm_socket_emitter_clean_file(struct vtm_socket_emitter *se)
{
	fclose(((struct vtm_emt_file*) se)->fp);
}

int vtm_socket_emitter_try_write(struct vtm_socket_emitter **se)
{
	int rc;
	enum vtm_socket_emitter_result res;
	struct vtm_socket_emitter *cur;
	struct vtm_socket_emitter *next;

	cur = *se;
	while (true) {
		res = cur->vtm_sock_emt_write(cur);
		switch (res) {
			case VTM_SOCK_EMIT_ERROR:
				rc = VTM_ERROR;
				goto out;

			case VTM_SOCK_EMIT_AGAIN:
				rc = VTM_E_IO_AGAIN;
				goto out;

			case VTM_SOCK_EMIT_COMPLETE:
				next = cur->next;
				vtm_socket_emitter_free_single(cur);
				cur = next;
				if (!cur) {
					rc = VTM_OK;
					goto out;
				}
				break;
		}
	}

out:
	*se = cur;
	return rc;
}

int vtm_socket_emitter_get_chain_lensum(struct vtm_socket_emitter *se, uint64_t *out_sum)
{
	uint64_t sum;

	sum = 0;
	while (se) {
		sum += se->length;
		if (sum < se->length)
			return vtm_err_set(VTM_E_OVERFLOW);
		se = se->next;
	}

	*out_sum = sum;
	return VTM_OK;
}
