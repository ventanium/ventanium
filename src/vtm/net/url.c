/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "url.h"

#include <string.h> /* memset() */
#include <vtm/core/error.h>
#include <vtm/core/format.h>
#include <vtm/core/string.h>

const char* VTM_URL_SCHEMES[] = {
	"https",
	"http",
	NULL
};

/* forward declaration */
static int vtm_url_parse_port(const char *colon, const char *slash, struct vtm_url *url);
static int vtm_get_hexchar_value(unsigned char hex, unsigned char *out);

int vtm_url_parse(const char *str, struct vtm_url *url)
{
	int rc;
	int scheme_idx;
	const char **scheme;
	const char *cur;
	const char *host_begin;
	const char *host_end;
	const char *first_slash;
	const char *first_colon;
	size_t diff;
	bool scan;

	memset(url, 0, sizeof(*url));

	/* parse scheme */
	scheme = VTM_URL_SCHEMES;
	scheme_idx = 0;

	while(*scheme != NULL) {
		if (vtm_str_starts_with(str, *scheme)) {
			url->scheme = scheme_idx;
			break;
		}
		scheme++;
		scheme_idx++;
	}

	if (*scheme == NULL)
		return VTM_E_NOT_SUPPORTED;

	/* check scheme separator */
	cur = str + strlen(*scheme);
	if (*cur++ != ':')
		return VTM_E_INVALID_ARG;
	if (*cur++ != '/')
		return VTM_E_NOT_SUPPORTED;
	if (*cur++ != '/')
		return VTM_E_NOT_SUPPORTED;

	/* scan input */
	host_begin = cur;
	first_slash = NULL;
	first_colon = NULL;

	scan = true;
	while(scan) {
		switch (*cur) {
			case '\0':
				scan = false;
				break;

			case ':':
				if (!first_colon && !first_slash)
					first_colon = cur;
				break;

			case '/':
				if (!first_slash)
					first_slash = cur;
				break;

			default:
				break;
		}
		cur++;
	}

	/* parse port */
	rc = vtm_url_parse_port(first_colon, first_slash, url);
	if (rc != VTM_OK)
		return rc;

	/* parse host */
	if (!first_colon && !first_slash) {
		url->host = vtm_str_copy(host_begin);
	}
	else {
		host_end = first_colon ? first_colon : first_slash;
		diff = host_end - host_begin;
		url->host = vtm_str_ncopy(host_begin, diff);
	}

	/* parse path */
	if (first_slash)
		url->path = vtm_str_copy(first_slash);

	return VTM_OK;
}

void vtm_url_release(struct vtm_url *url)
{
	free(url->host);
	free(url->path);
}

static int vtm_url_parse_port(const char *colon, const char *slash, struct vtm_url *url)
{
	char c;
	unsigned int old;
	unsigned int cnt;
	bool loop;

	/* parse port? */
	if (colon) {
		colon++;
		cnt = 0;
		loop = true;
		while (loop) {
			c = *colon++;
			switch (c) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					old = url->port;
					url->port = url->port * 10 + c - '0';
					if (url->port < old)
						return VTM_E_INVALID_ARG;
					break;

				default:
					loop = false;
					break;
			}
			cnt++;
		}

		if (cnt == 0)
			return VTM_E_INVALID_ARG;

		return VTM_OK;
	}

	/* set default port */
	switch (url->scheme) {
		case VTM_URL_SCHEME_HTTP:
			url->port = 80;
			break;

		case VTM_URL_SCHEME_HTTPS:
			url->port = 443;
			break;

		default:
			break;
	}

	return VTM_OK;
}

int vtm_url_encode(const char *s, void *buf, size_t len)
{
	unsigned char c, *out;
	size_t idx;

	out = buf;
	idx = 0;

	while ((c = (unsigned char) *s++) != '\0') {
		/* normal characters */
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_' ||
			c == '~') {
			if (idx >= len)
				return VTM_E_MAX_REACHED;
			out[idx++] = c;
			continue;
		}

		/* encode */
		if (idx + 2 >= len)
			return VTM_E_MAX_REACHED;

		out[idx++] = '%';
		out[idx++] = VTM_HEX_CHARS[c >> 4];
		out[idx++] = VTM_HEX_CHARS[c & 0xf];
	}

	if (idx >= len)
		return VTM_E_MAX_REACHED;

	out[idx] = '\0';

	return VTM_OK;
}

int vtm_url_decode(const char *s, void *buf, size_t len)
{
	int rc;
	const unsigned char *in;
	unsigned char c, t, *out;
	size_t idx;

	in = (const unsigned char*) s;
	out = buf;
	idx = 0;

	while ((c = *in++) != '\0') {
		if (idx >= len)
			return VTM_E_MAX_REACHED;

		/* normal characters */
		if (c != '%') {
			out[idx++] = c;
			continue;
		}

		/* decode */
		if (in[0] == '\0' || in[1] == '\0')
			return VTM_E_INVALID_ARG;

		rc = vtm_get_hexchar_value(in[0], &c);
		if (rc != VTM_OK)
			return VTM_E_INVALID_ARG;

		rc = vtm_get_hexchar_value(in[1], &t);
		if (rc != VTM_OK)
			return VTM_E_INVALID_ARG;

		out[idx++] = (c << 4) +  t;
		in += 2;
	}

	if (idx >= len)
		return VTM_E_MAX_REACHED;

	out[idx] = '\0';

	return VTM_OK;
}

static int vtm_get_hexchar_value(unsigned char hex, unsigned char *out)
{
	if (hex >= '0' && hex <= '9') {
		*out = hex - '0';
		return VTM_OK;
	}

	if (hex >= 'A' && hex <= 'F') {
		*out = 10 + hex - 'A';
		return VTM_OK;
	}

	if (hex >= 'a' && hex <= 'f') {
		*out = 10 + hex - 'a';
		return VTM_OK;
	}

	return VTM_E_INVALID_ARG;
}
