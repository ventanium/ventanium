/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_NET_HTTP_HTTP_ROUTE_H_
#define VTM_NET_HTTP_HTTP_ROUTE_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/net/http/http_context.h>
#include <vtm/net/http/http_memory.h>
#include <vtm/net/http/http_request.h>
#include <vtm/net/http/http_response.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vtm_http_route;

typedef int (*vtm_http_rt_handle_fn)(struct vtm_http_route *rt, struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res);
typedef void (*vtm_http_rt_free_fn)(struct vtm_http_route *rt);

struct vtm_http_route
{
	char                   *url_path;
	vtm_http_rt_handle_fn  fn_rt_handle;
	vtm_http_rt_free_fn    fn_rt_free;
};

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_ROUTE_H_ */
