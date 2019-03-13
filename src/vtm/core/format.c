/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "format.h"

const char VTM_HEX_CHARS[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

const char VTM_HEX_LCHARS[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

unsigned int vtm_fmt_int(char *dst, int val)
{
	if (val < 0) {
		if (dst)
			*dst++ = '-';
		return vtm_fmt_uint(dst, (unsigned int) -val) + 1;
	}
	
	return vtm_fmt_uint(dst, (unsigned int) val);
}

unsigned int vtm_fmt_uint(char *dst, unsigned int val)
{
	unsigned int n;
	unsigned int r;
	
	/* count chars */
	r = val;
	for (n=1; r > 9; n++)
		r /= 10;
		
	/* return length only? */
	if (!dst)
		return n;
		
	/* write chars */
	dst += n - 1;
	for (r=n; r > 0; r--, val /= 10)
		*dst-- = '0' + (val % 10);
		
	return n;
}

unsigned int vtm_fmt_int64(char *dst, int64_t val)
{
	if (val < 0) {
		if (dst)
			*dst++ = '-';
		return vtm_fmt_uint64(dst, (uint64_t) -val) + 1;
	}
	
	return vtm_fmt_uint64(dst, (uint64_t) val);
}

unsigned int vtm_fmt_uint64(char *dst, uint64_t val)
{
	unsigned int n;
	uint64_t r;
	
	/* count chars */
	r = val;
	for (n=1; r > 9; n++)
		r /= 10;
		
	/* return length only? */
	if (!dst)
		return n;
		
	/* write chars */
	dst += n - 1;
	for (r=n; r > 0; r--, val /= 10)
		*dst-- = '0' + (val % 10);
		
	return n;
}

unsigned int vtm_fmt_hex_size(char *dst, size_t val)
{
	unsigned int n;
	size_t r;
	
	/* count chars */
	r = val;
	for (n=1; r > 0x0f; n++)
		r = r >> 4;
		
	/* return length only? */
	if (!dst)
		return n;
		
	/* write chars */
	dst += n - 1;
	for (r=n; r > 0; r--, val = val >> 4) 
		*dst-- = VTM_HEX_CHARS[(val & 0x0f)];
	
	return n;
}

unsigned int vtm_fmt_hex_mem(char *dst, const unsigned char *src, size_t len, bool ucase)
{
	const char *t;
	unsigned int n;
	unsigned char c;
	
	if (len * 2 < len || len * 2 > UINT_MAX)
		return 0;
		
	n = (unsigned int) (len * 2);
	
	/* return length only? */
	if (!dst)
		return n;
	
	t = ucase ? VTM_HEX_CHARS : VTM_HEX_LCHARS;
	while (len-- > 0) {
		c = *src++;
		*dst++ = t[c >> 4];
		*dst++ = t[c & 0x0f];
	}
	
	return n;
}
