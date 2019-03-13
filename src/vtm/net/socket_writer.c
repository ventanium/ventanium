/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "socket_writer.h"

#include <string.h> /* memcpy(), strlen() */
#include <vtm/core/error.h>

/* forward declaration */
static int vtm_socket_writer_ensure_space(struct vtm_socket_writer *sw, size_t len);

void vtm_socket_writer_reset(struct vtm_socket_writer *sw)
{
	sw->index = 0;
	sw->lrc = VTM_OK;
}

int vtm_socket_writer_putm(struct vtm_socket_writer *sw, const char *src, size_t len)
{
	if (sw->lrc != VTM_OK)
		return sw->lrc;
	
	sw->lrc = vtm_socket_writer_ensure_space(sw, len);
	if (sw->lrc != VTM_OK)
		return sw->lrc;
		
	memcpy(sw->buf + sw->index, src, len);
	sw->index += len;

	return VTM_OK;
}

int vtm_socket_writer_puts(struct vtm_socket_writer *sw, const char *str)
{
	return vtm_socket_writer_putm(sw, str, strlen(str));
}

int vtm_socket_writer_putc(struct vtm_socket_writer *sw, char c)
{
	if (sw->lrc != VTM_OK)
		return sw->lrc;
	
	sw->lrc = vtm_socket_writer_ensure_space(sw, 1);
	if (sw->lrc != VTM_OK)
		return sw->lrc;
		
	sw->buf[sw->index++] = c;

	return VTM_OK;
}

int vtm_socket_writer_flush(struct vtm_socket_writer *sw)
{
	size_t written;
	
	if (sw->lrc != VTM_OK)
		return sw->lrc;
	
	sw->lrc = vtm_socket_write(sw->sock, sw->buf, sw->index, &written);
	if (sw->lrc != VTM_OK)
		return sw->lrc;
	
	if (written != sw->index) {
		sw->lrc = vtm_err_set(VTM_E_IO_UNKNOWN);
		return sw->lrc;
	}
	
	sw->index = 0;
	
	return VTM_OK;
}

static int vtm_socket_writer_ensure_space(struct vtm_socket_writer *sw, size_t len)
{
	if (sw->index + len >= sizeof(sw->buf))
		return vtm_socket_writer_flush(sw);
	
	return VTM_OK;
}
