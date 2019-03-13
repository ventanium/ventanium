/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "mime.h"

#include <string.h>
#include <ctype.h>
#include <vtm/core/string.h>
#include <vtm/fs/file.h>

const char* vtm_mime_type_for_name(const char *filename)
{
	const char *ext;

	ext = vtm_file_get_ext(filename);

	return vtm_mime_type_for_ext(ext);
}

const char* vtm_mime_type_for_ext(const char *ext)
{
	if (!ext)
		return NULL;

	switch (tolower(*ext)) {
		case 'h':
			if (vtm_str_casecmp(ext, "html") == 0)
				return VTM_MIME_TEXT_HTML;
			break;

		case 'j':
			if (vtm_str_casecmp(ext, "jpg") == 0 ||
				vtm_str_casecmp(ext, "jpeg") == 0)
				return VTM_MIME_IMAGE_JPEG;
			if (vtm_str_casecmp(ext, "json") == 0)
				return VTM_MIME_APP_JSON;
			if (vtm_str_casecmp(ext, "js") == 0)
				return VTM_MIME_APP_JAVASCRIPT;
			break;

		case 'p':
			if (vtm_str_casecmp(ext, "png") == 0)
				return VTM_MIME_IMAGE_PNG;
			break;

		case 't':
			if (vtm_str_casecmp(ext, "txt") == 0)
				return VTM_MIME_TEXT_PLAIN;
			break;

		default:
			break;
	}

	return NULL;
}
