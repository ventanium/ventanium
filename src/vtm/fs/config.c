/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "config.h"

#include <stdio.h> /* fopen(), fclose() */
#include <string.h> /* strlen() */
#include <vtm/core/error.h>
#include <vtm/core/string.h>
#include <vtm/fs/file.h>

/* forward declaration */
static void vtm_config_file_parse_line(char *line, size_t line_len, vtm_dataset *ds);

vtm_dataset* vtm_config_file_read(const char *file)
{
	FILE* fp;
	vtm_dataset *conf;
	char *buf;
	size_t buf_len;

	fp = fopen(file, "r");
	if (!fp) {
		vtm_err_setf(VTM_E_IO_FILE_NOT_FOUND, "could not open file: %s", file);
		return NULL;
	}

	conf = vtm_dataset_new();
	buf = NULL;
	buf_len = 0;

	while (vtm_file_getline(fp, &buf, &buf_len) == VTM_OK) {
		vtm_config_file_parse_line(buf, buf_len, conf);
	}

	free(buf);
	fclose(fp);
	return conf;
}

static void vtm_config_file_parse_line(char *line, size_t line_len, vtm_dataset *ds)
{
	char *act;
	char *key;
	char *val;
	vtm_list *split;

	for (act = line; *act != '\0'; act++) {
		switch (*act) {
			case '#':
			case ';':
				*act = '\0';
				goto parse;

			default:
				break;
		}
	}

parse:
	line = vtm_str_trim(line);
	if (strlen(line) == 0)
		return;

	split = vtm_str_split(line, "=", 1);
	if (vtm_list_size(split) != 2)
		goto exit;

	key = vtm_list_get_pointer(split, 0);
	val = vtm_list_get_pointer(split, 1);

	key = vtm_str_trim(key);
	val = vtm_str_trim(val);

	vtm_dataset_set_string(ds, key, val);

exit:
	vtm_list_free(split);
	return;
}
