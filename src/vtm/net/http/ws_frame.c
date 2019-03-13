/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "ws_frame_intl.h"

#include <vtm/core/error.h>
#include <vtm/net/http/ws_error.h>

int vtm_ws_frame_write_header(struct vtm_buf *buf, struct vtm_ws_frame_desc *desc)
{
	unsigned char header[14];
	unsigned int header_len;
	uint8_t len7;
	uint16_t len16;
	uint64_t len64;

	if (desc->len > VTM_WS_LEN64_MAX)
		return vtm_err_set(VTM_E_WS_INVALID_PAYLOAD_LEN);

	header_len = 0;

	/* fin and opcode */
	header[0]  = desc->fin << 7;
	header[0] |= desc->rsv1 << 6;
	header[0] |= desc->rsv2 << 5;
	header[0] |= desc->rsv3 << 4;
	header[0] |= desc->opcode & 0x0f;
	header_len++;

	/* mask and len7 */
	if (desc->len > VTM_WS_LEN16_MAX)
		len7 = VTM_WS_LEN64_ID;
	else if (desc->len > VTM_WS_LEN7_MAX)
		len7 = VTM_WS_LEN16_ID;
	else
		len7 = (uint8_t) desc->len;

	header[1]  = desc->masked << 7;
	header[1] |= len7 & 0x7f;
	header_len++;

	/* [len16] */
	if (len7 == VTM_WS_LEN16_ID) {
		len16 = (uint16_t) desc->len;
		header[2] = (len16 >> 8) & 0xff;
		header[3] = len16 & 0xff;
		header_len += 2;
	}

	/* [len64] */
	if (len7 == VTM_WS_LEN64_ID) {
		len64 = (uint64_t) desc->len;
		header[2] = (len64 >> 56) & 0xff;
		header[3] = (len64 >> 48) & 0xff;
		header[4] = (len64 >> 40) & 0xff;
		header[5] = (len64 >> 32) & 0xff;
		header[6] = (len64 >> 24) & 0xff;
		header[7] = (len64 >> 16) & 0xff;
		header[8] = (len64 >>  8) & 0xff;
		header[9] = len64 & 0xff;
		header_len +=8;
	}

	/* [mask] */
	if (desc->masked) {
		header[header_len++] = (desc->mask >> 24) & 0xff;
		header[header_len++] = (desc->mask >> 16) & 0xff;
		header[header_len++] = (desc->mask >> 8) & 0xff;
		header[header_len++] = desc->mask & 0xff;
	}

	return vtm_buf_putm(buf, header, header_len);
}

void vtm_ws_frame_mask_payload(void *dst, size_t len, uint32_t mask)
{
	size_t i;
	unsigned char *dst_uc;
	unsigned char mask_uc[4];

	dst_uc = dst;
	for (i=0; i < 4; i++)
		mask_uc[i] = (mask >> (24-8*i)) & 0xff;

	for (i=0; i < len; i++) {
		dst_uc[i] = dst_uc[i] ^ mask_uc[i % 4];
	}
}
