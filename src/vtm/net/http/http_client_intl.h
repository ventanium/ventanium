/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_NET_HTTP_HTTP_CLIENT_INTL_H_
#define VTM_NET_HTTP_HTTP_CLIENT_INTL_H_

#include <vtm/net/http/http_client.h>

#ifdef __cplusplus
extern "C" {
#endif

int vtm_http_client_take_socket(vtm_http_client *cl, vtm_socket **sock);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_CLIENT_INTL_H_ */
