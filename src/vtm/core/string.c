/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "string.h"

#include <stdlib.h> /* free() */
#include <stdio.h>  /* vsnprintf() */
#include <string.h> /* strlen(), strtol(), strtoul(), strtod(), strncmp(), memmove() */
#include <ctype.h>  /* isspace(), tolower() */

#include <vtm/core/error.h>
#include <vtm/core/math.h>

/* vtm_strdup() */
#ifdef VTM_HAVE_POSIX
#define vtm_strdup      strdup
#else
#define vtm_strdup      vtm_strdup_self
#endif

/* vtm_strtok_r() */
#ifdef VTM_HAVE_POSIX
#define vtm_strtok_r    strtok_r
#else
#define vtm_strtok_r    vtm_strtok_r_self
#endif

#ifndef VTM_HAVE_POSIX
static char* vtm_strdup_self(const char* in)
{
	char *copy;
	size_t len;

	len = strlen(in);
	copy = malloc(len+1);
	if (!copy) {
		vtm_err_oom();
		return NULL;
	}

	memcpy(copy, in, len+1);

	return copy;
}

static char* vtm_strtok_r_self(char* in, const char *delim, char **ctx)
{
	int rc;
	size_t len;

	if (!in)
		in = *ctx;

	in += strspn(in, delim);
	if (*in == '\0')
		return NULL;

	len = strcspn(in, delim);
	if (in[len] != '\0') {
		in[len] = '\0';
		rc = vtm_math_size_add(len, strlen(delim), &len);
		if (rc != VTM_OK)
			VTM_ABORT("Overflow");
	}

	*ctx = in + len;

	return in;
}
#endif /* NOT VTM_HAVE_POSIX */

char* vtm_str_copy(const char *in)
{
	char *copy;

	copy = vtm_strdup(in);
	if (!copy)
		vtm_err_oom();

	return copy;
}

char* vtm_str_ncopy(const char *in, size_t len)
{
	char *copy;

	copy = malloc(len + 1);
	if (!copy) {
		vtm_err_oom();
		return NULL;
	}

	memcpy(copy, in, len);
	copy[len] = '\0';

	return copy;
}

int vtm_str_casecmp(const char *s1, const char *s2)
{
	while (tolower(*s1) == tolower(*s2)) {
		if (*s1 == '\0' || *s2 == '\0')
			break;
		s1++;
		s2++;
	}

	return tolower(*s1) - tolower(*s2);
}

vtm_list* vtm_str_split(const char *in, const char *delim, unsigned int count)
{
	vtm_list *list = vtm_list_new(VTM_ELEM_STRING, 8);
	vtm_list_set_free_func(list, free);

	unsigned int tokens = 0;
	char *save = NULL;
	char *copy = vtm_str_copy(in);
	char *tok = vtm_strtok_r(copy, delim, &save);

	while (tok != NULL) {
		vtm_list_add_va(list, vtm_str_copy(tok));
		tokens++;

		if ( count > 0 && tokens == count )
			delim = "";

		tok = vtm_strtok_r(NULL, delim, &save);
	}

	free(copy);
	return list;
}

int vtm_str_split_ex(const char *in, const char *delim, char ***out_tokens, size_t *token_count)
{
	int rc;
	size_t tokens;
	size_t buf_size;
	char *save;
	char *copy;
	char *tok;
	char *end;
	char **buf;

	rc = VTM_OK;
	tokens = 0;

	buf_size = vtm_str_pattern_count(in, delim) + 1;
	buf = calloc(buf_size, sizeof(char*));
	if (!buf) {
		vtm_err_oom();
		rc = vtm_err_get_code();
		goto result;
	}

	save = NULL;
	copy = vtm_str_copy(in);
	tok = vtm_strtok_r(copy, delim, &save);

	while (tok != NULL) {
		while (isspace(*tok))
			tok++;

		end = tok;
		while (*end != '\0') {
			if (isspace(*end))
				*end = '\0';
			end++;
		}

		if (end != tok)
			buf[tokens++] = vtm_str_copy(tok);
		tok = vtm_strtok_r(NULL, delim, &save);
	}

	free(copy);

result:
	*out_tokens = buf;
	*token_count = tokens;

	return rc;
}

char* vtm_str_trim(char *in)
{
	char *begin = in;
	char *act = in;
	for (; *act != '\0'; act++) {
		if ( !isspace(*act) ) {
			begin = act;
			break;
		}
	}

	char *end = act;
	for (; *act != '\0'; act++) {
		if ( !isspace(*act) )
			end=act+1;
	}

	size_t newlen = end-begin;
	memmove(in, begin, newlen);
	in[newlen] = '\0';

	return in;
}

char* vtm_str_printf(const char *fmt, ...)
{
	char *str;
	va_list ap;

	va_start(ap, fmt);
	str = vtm_str_vprintf(fmt, ap);
	va_end(ap);

	return str;
}

char* vtm_str_vprintf(const char *fmt, va_list ap)
{
	int rc;
	char *str;
	size_t last_size;
	size_t needed_size;
	va_list cp;

	str = NULL;
	last_size = strlen(fmt);
	needed_size = last_size;
	while (needed_size >= last_size ) {
		va_copy(cp, ap);
		free(str);
		rc = vtm_math_size_mul(last_size, 2, &last_size);
		if (rc != VTM_OK)
			return NULL;
		str = malloc(last_size);
		needed_size = vsnprintf(str, last_size, fmt, cp);
		va_end(cp);
	}
	return str;
}

ssize_t vtm_str_index_of(const char *in, char c, size_t len)
{
	size_t i;

	if (len > SSIZE_MAX)
		return -2;

	for (i=0; *in != '\0' && i < len; i++, in++) {
		if (*in == c)
			return i;
	}

	return -1;
}

ssize_t vtm_str_index_chars(const char *in, size_t len, char *chars, size_t char_count, char *out_found)
{
	const char *cur;
	const char *end;
	size_t i;
	char c;

	cur = in;
	end = in + len;

	while (cur != end) {
		c = *cur;
		for (i=0; i < char_count; i++) {
			if (c == chars[i]) {
				*out_found = c;
				return cur - in;
			}
		}

		cur++;
	}

	return -1;
}

ssize_t vtm_str_index_pattern(const char *in, const char *pattern, size_t input_len, size_t pattern_len)
{
	size_t i, c, begin;
	bool found;

	if (input_len > SSIZE_MAX)
		return -2;

	begin = pattern_len -1;
	for (i=begin; in[i] != '\0' && i < input_len; i++) {
		found = true;
		for (c=0; c < pattern_len; c++) {
			if (in[i-(begin - c)] != pattern[c]) {
				found = false;
				break;
			}
		}
		if (found)
			return i-begin;
	}

	return -1;
}

bool vtm_str_list_contains(const char *in, const char *delim, const char *search, bool nocase)
{
	bool result;
	char *save;
	char *copy;
	char *tok;
	char *end;

	result = false;
	save = NULL;
	copy = vtm_str_copy(in);

	tok = vtm_strtok_r(copy, delim, &save);
	while (tok != NULL){

		while (isspace(*tok))
			tok++;

		end = tok;
		while (*end != '\0') {
			if (isspace(*end))
				*end = '\0';
			end++;
		}

		if ((nocase && vtm_str_casecmp(search, tok) == 0) ||
			(!nocase && strcmp(search, tok) == 0)) {
			result = true;
			goto found;
		}

		tok = vtm_strtok_r(NULL, delim, &save);
	}

found:
	free(copy);
	return result;
}

size_t vtm_str_pattern_count(const char *in, const char *pattern)
{
	size_t result;
	size_t pattern_len;
	size_t i;

	result = 0;
	pattern_len = strlen(pattern);
	while (*in != '\0') {
		for (i=0; i < pattern_len && *in != '\0'; i++) {
			if(*in++ != pattern[i])
				break;
		}

		if (i == pattern_len)
			result++;
	}

	return result;
}

bool vtm_str_starts_with(const char *in, const char *start)
{
	return strncmp(start, in, strlen(start)) == 0;
}
