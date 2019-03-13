/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file http_parser.h
 *
 * @brief HTTP parser
 */

#ifndef VTM_NET_HTTP_HTTP_PARSER_H_
#define VTM_NET_HTTP_HTTP_PARSER_H_

#include <vtm/core/api.h>
#include <vtm/core/buffer.h>
#include <vtm/core/dataset.h>
#include <vtm/core/types.h>
#include <vtm/net/common.h>
#include <vtm/net/http/http.h>

#ifdef __cplusplus
extern "C" {
#endif

/** default maximum header size in bytes */
#define VTM_HTTP_DEF_MAX_HEADER_SIZE          8192
/** default maximum body size in bytes */
#define VTM_HTTP_DEF_MAX_BODY_SIZE    (8*1024*1024)

enum vtm_http_parser_mode
{
	VTM_HTTP_PM_REQUEST,    /**< Parse client request */
	VTM_HTTP_PM_RESPONSE    /**< Parse server response */
};

enum vtm_http_parser_state
{
	VTM_HTTP_PARSE_BEGIN,
	VTM_HTTP_PARSE_REQ_METHOD,
	VTM_HTTP_PARSE_REQ_PATH,
	VTM_HTTP_PARSE_REQ_PARAM_BEGIN,
	VTM_HTTP_PARSE_REQ_PARAM_NAME,
	VTM_HTTP_PARSE_REQ_PARAM_VALUE,
	VTM_HTTP_PARSE_REQ_LINE_LF,
	VTM_HTTP_PARSE_RES_STATUS_CODE,
	VTM_HTTP_PARSE_RES_STATUS_MSG_CR,
	VTM_HTTP_PARSE_RES_STATUS_MSG_LF,
	VTM_HTTP_PARSE_VERSION_H,
	VTM_HTTP_PARSE_VERSION_HT,
	VTM_HTTP_PARSE_VERSION_HTT,
	VTM_HTTP_PARSE_VERSION_HTTP,
	VTM_HTTP_PARSE_VERSION_SLASH,
	VTM_HTTP_PARSE_VERSION_MAJOR,
	VTM_HTTP_PARSE_VERSION_MINOR,
	VTM_HTTP_PARSE_HEADER_LINE_BEGIN,
	VTM_HTTP_PARSE_HEADER_NAME,
	VTM_HTTP_PARSE_HEADER_VALUE,
	VTM_HTTP_PARSE_HEADER_LINE_LF,
	VTM_HTTP_PARSE_HEADER_LINE_COMPLETE,
	VTM_HTTP_PARSE_HEADERS_END_LF,
	VTM_HTTP_PARSE_BODY,
	VTM_HTTP_PARSE_BODY_READALL,
	VTM_HTTP_PARSE_BODY_FIXEDLENGTH,
	VTM_HTTP_PARSE_BODY_CHUNKED,
	VTM_HTTP_PARSE_BODY_CHUNK_SIZE,
	VTM_HTTP_PARSE_BODY_CHUNK_SIZE_LF,
	VTM_HTTP_PARSE_BODY_CHUNK_CONTENT,
	VTM_HTTP_PARSE_BODY_CHUNK_END_LF,
	VTM_HTTP_PARSE_COMPLETE
};

struct vtm_http_parser
{
	enum vtm_http_parser_mode    mode;
	enum vtm_http_parser_state   state;

	/* limits */
	size_t                       max_header_size;
	size_t                       max_body_size;

	/* request fields */
	enum vtm_http_method         req_method;
	const char                  *req_path;
	vtm_dataset                 *req_params;
	bool                         req_params_free;

	/* response fields */
	int                          res_status_code;
	const char                  *res_status_msg;

	/* shared fields */
	enum vtm_http_version        version;
	vtm_dataset                 *headers;
	bool                         headers_free;
	void                        *body;
	uint64_t                     body_len;

	/* internal fields */
	unsigned int                 version_major;
	unsigned int                 version_minor;
	size_t                       state_chars;
	size_t                       status_msg_begin;
	size_t                       path_begin;

	size_t                       param_name_begin;

	size_t                       header_name_begin;
	size_t                       header_value_begin;

	size_t                       body_begin;
	size_t                       chunk_dst;
	size_t                       chunk_begin;
	size_t                       chunk_size;
};

/**
 * Initializes the parser for the given mode.
 *
 * @param par the parser that should be initialized
 * @param mode the mode under which the parser should run
 */
VTM_API void vtm_http_parser_init(struct vtm_http_parser *par, enum vtm_http_parser_mode mode);

/**
 * Releases the parser and all allocated resources.
 *
 * After this call the parser is no longer valid.
 *
 * @param par the parser that should be released
 */
VTM_API void vtm_http_parser_release(struct vtm_http_parser *par);

/**
 * Resets the parser for parsing a new request or response.
 *
 * @param par the parser that should be reset
 */
VTM_API void vtm_http_parser_reset(struct vtm_http_parser *par);

/**
 * Lets the parser examine the input data.
 *
 * @param par the parser the should run
 * @param buf the input buffer for the parser
 * @return VTM_NET_RECV_STAT_COMPLETE if a request or response was
 *         successfully parsed
 * @return VTM_NET_RECV_STAT_AGAIN if the parser need a another run with more
 *         input data
 * @return VTM_NET_RECV_STAT_INVALID if the input data is not a valid request
 *         or response
 * @return VTM_NET_RECV_STAT_ERROR if an error occured for example a necessary
 *         buffer could not be allocated
 */
VTM_API enum vtm_net_recv_stat vtm_http_parser_run(struct vtm_http_parser *par, struct vtm_buf *buf);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_PARSER_H_ */
