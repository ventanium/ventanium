/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_NET_HTTP_WS_MESSAGE_INTL_H_
#define VTM_NET_HTTP_WS_MESSAGE_INTL_H_

#include <vtm/net/http/ws_message.h>

#ifdef __cplusplus
extern "C" {
#endif

void vtm_ws_msg_init(struct vtm_ws_msg *msg, enum vtm_ws_msg_type type, void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_WS_MESSAGE_INTL_H_ */
