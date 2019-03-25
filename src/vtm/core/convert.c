/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "convert.h"

#include <stdlib.h> /* strtol(), strtoul(), strtod() */
#include <vtm/core/blob.h>
#include <vtm/core/format.h>
#include <vtm/core/string.h>

char* vtm_conv_int64_str(int64_t num)
{
	return vtm_str_printf("%" PRId64, num);
}

char* vtm_conv_uint64_str(uint64_t num)
{
	return vtm_str_printf("%" PRIu64, num);
}

char* vtm_conv_double_str(double r)
{
	return vtm_str_printf("%g", r);
}

char* vtm_conv_blob_str(void *blob)
{
	size_t len;
	char *str;

	len = vtm_blob_size(blob);
	if (len * 2 < len)
		return NULL;

	str = malloc((len * 2) + 1);
	if (!str)
		return NULL;

	str[vtm_fmt_hex_mem(str, blob, len, true)] = '\0';

	return str;
}

int64_t vtm_conv_str_int64(const char *in)
{
	return strtoll(in, NULL, 10);
}

uint64_t vtm_conv_str_uint64(const char *in)
{
	return strtoull(in, NULL, 10);
}

double vtm_conv_str_double(const char *in)
{
	return strtod(in, NULL);
}
