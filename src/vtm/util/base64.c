/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "base64.h"

#include <vtm/core/error.h>

#define VTM_BASE64_PADDING_CHAR '='

static const char VTM_BASE64_ENCODING[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3',	'4', '5', '6', '7', '8', '9', '+', '/'
};

static const char VTM_BASE64_DECODING[256] = {
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,
	 0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,
	 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

int vtm_base64_encode(const void *input, size_t len, void *buf, size_t buf_len)
{
	unsigned const char *r;
	unsigned char *w;
	uint32_t c;

	if (buf_len < VTM_BASE64_ENC_BUF_LEN(len))
		return vtm_err_set(VTM_E_INVALID_ARG);

	r = input;
	w = buf;
	while (len >= 3) {
		c  = r[0] << 16;
		c += r[1] << 8;
		c += r[2];

		w[0] = VTM_BASE64_ENCODING[c >> 18];
		w[1] = VTM_BASE64_ENCODING[(c >> 12) & 0x3F];
		w[2] = VTM_BASE64_ENCODING[(c >> 6) & 0x3F];
		w[3] = VTM_BASE64_ENCODING[c & 0x3F];

		len -= 3;
		r += 3;
		w += 4;
	}

	if (len > 0) {
		c = r[0] << 16;

		if (len > 1)
			c += r[1] << 8;

		*w++ = VTM_BASE64_ENCODING[c >> 18];
		*w++ = VTM_BASE64_ENCODING[(c >> 12) & 0x3F];
		*w++ = (len > 1) ? VTM_BASE64_ENCODING[(c >> 6) & 0x3F] : VTM_BASE64_PADDING_CHAR;
		*w++ = VTM_BASE64_PADDING_CHAR;
	}

	*w = '\0';

	return VTM_OK;
}

int vtm_base64_decode(const void *input, size_t len, void *buf, size_t *buf_len)
{
	const unsigned char *r;
	unsigned char *w;
	uint32_t c;
	int pad;
	size_t remaining;

	if (len < 4)
		return vtm_err_set(VTM_E_INVALID_ARG);

	if (*buf_len < VTM_BASE64_DEC_BUF_LEN(len))
		return vtm_err_set(VTM_E_INVALID_ARG);

	r = input;
	w = buf;

	pad  = r[len-1] == VTM_BASE64_PADDING_CHAR ? 1 : 0;
	pad += r[len-2] == VTM_BASE64_PADDING_CHAR ? 1 : 0;

	remaining = pad > 0 ? len - 4 : len;
	while (remaining > 0) {
		c  = ((uint32_t) VTM_BASE64_DECODING[r[0]]) << 18;
		c += ((uint32_t) VTM_BASE64_DECODING[r[1]]) << 12;
		c += ((uint32_t) VTM_BASE64_DECODING[r[2]]) << 6;
		c += ((uint32_t) VTM_BASE64_DECODING[r[3]]) << 0;

		w[0] = c >> 16 & 0xff;
		w[1] = c >> 8 & 0xff;
		w[2] = c >> 0 & 0xff;

		remaining -= 4;
		r += 4;
		w += 3;
	}

	if (pad) {
		c  = ((uint32_t) VTM_BASE64_DECODING[r[0]]) << 18;
		c += ((uint32_t) VTM_BASE64_DECODING[r[1]]) << 12;

		if (pad == 1)
			c += ((uint32_t) VTM_BASE64_DECODING[r[2]]) << 6;

		*w++ = c >> 16 & 0xff;

		if (pad == 1)
			*w++ = c >> 8 & 0xff;
	}

	*buf_len = VTM_BASE64_DEC_BUF_LEN(len) - pad;

	return VTM_OK;
}
