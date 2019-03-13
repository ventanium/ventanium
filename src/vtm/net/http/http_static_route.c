/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "http_static_route.h"

#include <stdlib.h> /* malloc() */
#include <vtm/core/error.h>

struct vtm_http_static_rt
{
	struct vtm_http_route         base;
	vtm_http_static_rt_handle_fn  fn_rt_handle;
};

/* forward declaration */
static int vtm_http_static_rt_handle(struct vtm_http_route *rt, struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res);
static void vtm_http_static_rt_free(struct vtm_http_route *rt);

struct vtm_http_route* vtm_http_static_rt_new(const char *url_path, vtm_http_static_rt_handle_fn fn_handle)
{
	struct vtm_http_static_rt *rt;

	rt = malloc(sizeof(*rt));
	if (!rt) {
		vtm_err_oom();
		return NULL;
	}

	rt->base.fn_rt_handle = vtm_http_static_rt_handle;
	rt->base.fn_rt_free = vtm_http_static_rt_free;

	rt->fn_rt_handle = fn_handle;

	return &rt->base;
}

static int vtm_http_static_rt_handle(struct vtm_http_route *rt, struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res)
{
	return ((struct vtm_http_static_rt*) rt)->fn_rt_handle(ctx, req, res);
}

static void vtm_http_static_rt_free(struct vtm_http_route *rt)
{
	free(rt);
}
