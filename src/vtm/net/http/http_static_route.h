/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file http_static_route.h
 *
 * @brief Route for programmed responses
 */

#ifndef VTM_NET_HTTP_HTTP_STATIC_ROUTE_H_
#define VTM_NET_HTTP_HTTP_STATIC_ROUTE_H_

#include <vtm/core/api.h>
#include <vtm/net/http/http_route.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*vtm_http_static_rt_handle_fn)(struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res);

/**
 * Creates a route for programmed responses.
 *
 * @param url_path the path where the route is bound
 * @param fn_handle the function that is called upon a request
 * @return http route if call succeeded
 * @return NULL if an error occured
 */
VTM_API struct vtm_http_route* vtm_http_static_rt_new(const char *url_path, vtm_http_static_rt_handle_fn fn_handle);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_STATIC_ROUTE_H_ */
