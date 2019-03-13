/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file ws_parser.h
 *
 * @brief WebSocket protocol parser
 */

#ifndef VTM_NET_HTTP_WS_PARSER_H_
#define VTM_NET_HTTP_WS_PARSER_H_

#include <vtm/core/api.h>
#include <vtm/core/buffer.h>
#include <vtm/net/common.h>
#include <vtm/net/http/ws.h>
#include <vtm/net/http/ws_message.h>

#ifdef __cplusplus
extern "C" {
#endif

enum vtm_ws_parser_stage
{
	VTM_WS_PARSE_MSG_BEGIN,
	VTM_WS_PARSE_FRAME_BEGIN,
	VTM_WS_PARSE_FRAME_FIN_OPCODE,
	VTM_WS_PARSE_FRAME_MASK_LEN7,
	VTM_WS_PARSE_FRAME_LEN16,
	VTM_WS_PARSE_FRAME_LEN64,
	VTM_WS_PARSE_FRAME_MASK32,
	VTM_WS_PARSE_FRAME_PAYLOAD,
	VTM_WS_PARSE_FRAME_FINISH_CTRL,
	VTM_WS_PARSE_FRAME_FINISH_DATA,
	VTM_WS_PARSE_FRAME_COMPLETE,
	VTM_WS_PARSE_MSG_COMPLETE,
	VTM_WS_PARSE_ERROR
};

struct vtm_ws_parser
{
	enum vtm_ws_mode mode;
	enum vtm_ws_parser_stage stage;

	/* frame data */
	unsigned int fin    : 1;
	unsigned int rsv1   : 1;
	unsigned int rsv2   : 1;
	unsigned int rsv3   : 1;
	unsigned int opcode : 4;
	unsigned int masked : 1;

	size_t payload_len;
	size_t payload_begin;
	uint32_t mask;

	/* msg assembly buffer */
	unsigned char *msg_buf;
	size_t msg_buf_size;
	size_t msg_buf_used;
	size_t msg_frame_count;
	enum vtm_ws_msg_type msg_type;

	/* interleaved control msg */
	bool has_ctrl_msg;
	struct vtm_ws_msg ctrl_msg;
};

/**
 * Initializes a new parser for given mode.
 *
 * @param par the parser that should be initialized
 * @param mode the mode under which the parser should run
 * @return VTM_OK if the parser was successfully initialized
 * @return VTM_ERROR if the parser could not be initialized
 */
VTM_API int vtm_ws_parser_init(struct vtm_ws_parser *par, enum vtm_ws_mode mode);

/**
 * Releases the parser and all allocated resources.
 *
 * After this call the parser structure is no longer valid.
 *
 * @param par the parser that should be released
 */
VTM_API void vtm_ws_parser_release(struct vtm_ws_parser *par);

/**
 * Resets the parser to a clean state.
 *
 * @param par the parser that should be reset
 */
VTM_API void vtm_ws_parser_reset(struct vtm_ws_parser *par);

/**
 * Lets the parser examine the input data.
 *
 * @param par the parser the should run
 * @param  buf the input buffer for the parser
 * @return VTM_NET_RECV_STAT_COMPLETE if a message was successfully parsed
 * @return VTM_NET_RECV_STAT_AGAIN if the parser need a another run with more
 *         input data
 * @return VTM_NET_RECV_STAT_INVALID if the input data is not a valid
 *         WebSocket message
 * @return VTM_NET_RECV_STAT_ERROR if an error occured for example a necessary
 *         buffer could not be allocated
 */
VTM_API enum vtm_net_recv_stat vtm_ws_parser_run(struct vtm_ws_parser *par, struct vtm_buf *buf);

/**
 * Retrieves the last parsed message
 *
 * @param par the parser
 * @param[out] msg the message is stored here
 * @return VTM_OK if the parsed message was successfully retrieved
 * @return VTM_ERROR if an error occured, for example a necessary buffer
 *         could not be allocated
 */
VTM_API int vtm_ws_parser_get_msg(struct vtm_ws_parser *par, struct vtm_ws_msg *msg);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_WS_PARSER_H_ */
