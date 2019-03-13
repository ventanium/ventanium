/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_NET_HTTP_WS_H_
#define VTM_NET_HTTP_WS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define VTM_WS_FIN_MORE             0
#define VTM_WS_FIN_FINAL            1

#define VTM_WS_MASK_DISABLED        0
#define VTM_WS_MASK_ENABLED         1

#define VTM_WS_OPCODE_CONTINUE      0x0
#define VTM_WS_OPCODE_TEXT          0x1
#define VTM_WS_OPCODE_BINARY        0x2
#define VTM_WS_OPCODE_CLOSE         0x8
#define VTM_WS_OPCODE_PING          0x9
#define VTM_WS_OPCODE_PONG          0xA

#define VTM_WS_LEN7_MAX             125
#define VTM_WS_LEN16_MAX            65535
#define VTM_WS_LEN64_MAX            9223372036854775807

#define VTM_WS_LEN16_ID             126
#define VTM_WS_LEN64_ID             127

enum vtm_ws_mode
{
	VTM_WS_MODE_SERVER,
	VTM_WS_MODE_CLIENT
};

enum vtm_ws_msg_type
{
	VTM_WS_MSG_TEXT    = VTM_WS_OPCODE_TEXT,
	VTM_WS_MSG_BINARY  = VTM_WS_OPCODE_BINARY,
	VTM_WS_MSG_CLOSE   = VTM_WS_OPCODE_CLOSE,
	VTM_WS_MSG_PING    = VTM_WS_OPCODE_PING,
	VTM_WS_MSG_PONG    = VTM_WS_OPCODE_PONG
};

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_WS_H_ */
