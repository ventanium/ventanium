/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_NET_HTTP_WS_FRAME_H_
#define VTM_NET_HTTP_WS_FRAME_H_

#include <vtm/core/types.h> /* size_t */
#include <vtm/core/buffer.h>
#include <vtm/net/http/ws.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vtm_ws_frame_desc
{
	unsigned int fin    : 1;
	unsigned int rsv1   : 1;
	unsigned int rsv2   : 1;
	unsigned int rsv3   : 1;
	unsigned int opcode : 4;
	unsigned int masked : 1;

	uint32_t mask;
	size_t len;
};

int vtm_ws_frame_write_header(struct vtm_buf *buf, struct vtm_ws_frame_desc *desc);
void vtm_ws_frame_mask_payload(void *dst, size_t len, uint32_t mask);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_WS_FRAME_H_ */
