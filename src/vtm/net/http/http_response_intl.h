/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_NET_HTTP_HTTP_RESPONSE_INTL_H_
#define VTM_NET_HTTP_HTTP_RESPONSE_INTL_H_

#include <vtm/net/socket.h>
#include <vtm/net/http/http.h>
#include <vtm/net/http/http_connection_intl.h>
#include <vtm/net/http/http_request.h>
#include <vtm/net/http/http_response.h>

#ifdef __cplusplus
extern "C" {
#endif

vtm_http_res* vtm_http_res_new(void);
void vtm_http_res_free(vtm_http_res *res);

void vtm_http_res_prepare(vtm_http_res *res, struct vtm_http_req *req);
enum vtm_http_res_act vtm_http_res_get_action(vtm_http_res *res);
void* vtm_http_res_get_action_data(vtm_http_res *res);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_RESPONSE_INTL_H_ */
