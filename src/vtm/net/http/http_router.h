/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file http_router.h
 *
 * @brief HTTP router
 */

#ifndef VTM_NET_HTTP_HTTP_ROUTER_H_
#define VTM_NET_HTTP_HTTP_ROUTER_H_

#include <vtm/core/api.h>
#include <vtm/net/http/http_context.h>
#include <vtm/net/http/http_route.h>
#include <vtm/net/http/http_static_route.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtm_http_router vtm_http_router;

/**
 * Creates a new router.
 *
 * @return the created router
 * @return NULL if an error occured
 */
VTM_API vtm_http_router* vtm_http_router_new(void);

/**
 * Releases the router and all allocated resources.
 *
 * This call also releases all routes that were added to the router.
 * After this call the router pointer is no longer valid.
 *
 * @param rtr the router that should be released
 */
VTM_API void vtm_http_router_free(vtm_http_router *rtr);

/**
 * Binds the route to the given URL path.
 *
 * @param rtr the router where the route should be added
 * @param path the URL path where the route should be bound to
 * @param rt the route that should be added
 * @return VTM_OK if the route was added successfully
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_http_router_add_rt(vtm_http_router *rtr, const char *path, struct vtm_http_route *rt);

/**
 * Adds a static route for the given path.
 *
 * @param rtr the router where the route should be added
 * @param path the URL path where the route should be bound to
 * @param fn the callback function that should handle the route
 * @return VTM_OK if the route was added successfully
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_http_router_static_rt(vtm_http_router *rtr, const char *path, vtm_http_static_rt_handle_fn fn);

/**
 * Lets the router handle a given request.
 *
 * @param rtr the router that should handle a request
 * @param ctx the context of the request
 * @param req the request that should be handled
 * @param res the response that musst be prepared
 * @return VTM_OK if the request was handled by the router
 * @return VTM_E_NOT_HANDLED if the request was not handled by any route
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured while handling
 *         the request
 */
VTM_API int vtm_http_router_handle(vtm_http_router *rtr, struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_ROUTER_H_ */
