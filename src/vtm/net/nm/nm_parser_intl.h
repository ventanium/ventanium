/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_NET_NM_NM_PARSER_INTL_H_
#define VTM_NET_NM_NM_PARSER_INTL_H_

#include <vtm/core/buffer.h>
#include <vtm/core/system.h>
#include <vtm/core/dataset.h>
#include <vtm/net/common.h>

#ifdef __cplusplus
extern "C" {
#endif

enum vtm_nm_parser_state
{
	VTM_NM_PARSE_MSG_BEGIN,
	VTM_NM_PARSE_MAGIC,
	VTM_NM_PARSE_VERSION,
	VTM_NM_PARSE_FIELD_COUNT,
	VTM_NM_PARSE_FIELD_BEGIN,
	VTM_NM_PARSE_NAME_LEN,
	VTM_NM_PARSE_NAME,
	VTM_NM_PARSE_VALUE_TYPE,
	VTM_NM_PARSE_VALUE_LEN,
	VTM_NM_PARSE_VALUE,
	VTM_NM_PARSE_FIELD_COMPLETE,
	VTM_NM_PARSE_MSG_COMPLETE
};

struct vtm_nm_parser
{
	enum vtm_byteorder        order;
	enum vtm_nm_parser_state  state;
	vtm_dataset               *msg;

	uint16_t                  field_count;
	uint16_t                  fields_parsed;

	uint8_t                   name_len;
	char*                     name;

	enum vtm_elem_type        value_type;
	uint32_t                  value_len;
	union vtm_elem            value;
};

void vtm_nm_parser_init(struct vtm_nm_parser *par);
void vtm_nm_parser_release(struct vtm_nm_parser *par);

void vtm_nm_parser_reset(struct vtm_nm_parser *par);
enum vtm_net_recv_stat vtm_nm_parser_run(struct vtm_nm_parser *par, struct vtm_buf *buf);
vtm_dataset* vtm_nm_parser_get_msg(struct vtm_nm_parser *par);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_NM_NM_PARSER_INTL_H_ */
