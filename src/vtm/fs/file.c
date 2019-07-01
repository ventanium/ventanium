/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "file.h"

#include <stdlib.h> /* malloc(), realloc() */
#include <vtm/core/error.h>

#define VTM_FILE_BUF_SIZE    256

uint64_t vtm_file_get_fsize(FILE *fp)
{
	uint64_t result;

	fseek(fp, 0L, SEEK_END);
	result = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	return result;
}

int vtm_file_getline(FILE *fp, char **buf, size_t *buf_len)
{
	char *cur;
	char *end;
	int c;
	size_t old;

	if (*buf == NULL || *buf_len == 0) {
		*buf = malloc(VTM_FILE_BUF_SIZE);
		if (!*buf) {
			vtm_err_oom();
			return vtm_err_get_code();
		}
		*buf_len = VTM_FILE_BUF_SIZE;
	}

	for (cur = *buf, end = *buf + *buf_len;;) {
		c = fgetc(fp);
		switch (c) {
			case EOF:
				if (cur == *buf)
					return VTM_E_IO_EOF;
				goto complete;

			case '\r':
				continue;

			case '\n':
				goto complete;

			default:
				break;
		}

		*cur++ = c;

		if (cur == end) {
			old = *buf_len;
			*buf_len = *buf_len * 2;
			*buf = realloc(*buf, *buf_len);
			if (!*buf) {
				vtm_err_oom();
				return vtm_err_get_code();
			}
			cur = *buf + old;
			end = *buf + *buf_len;
		}
	}

complete:
	*cur = '\0';

	return VTM_OK;
}

const char* vtm_file_get_ext(const char *filename)
{
	const char *p;
	size_t last;

	last = 0;
	for(p=filename; *p != '\0'; p++) {
		if (*p == '.')
			last = p - filename;
	}

	if (last == 0)
		return NULL;

	return filename + last + 1;
}
