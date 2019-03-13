/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "http_router.h"

#include <string.h> /* strcmp() */
#include <vtm/core/error.h>
#include <vtm/core/list.h>
#include <vtm/core/string.h>
#include <vtm/net/http/http_request.h>
#include <vtm/net/http/http_static_route.h>

struct vtm_http_router
{
	vtm_list *routes;
};

/* forward declaration */
static bool vtm_http_router_matches(const char *path, const char *rt_path);
static void vtm_http_router_http_free_rt(void *val);

vtm_http_router* vtm_http_router_new(void)
{
	vtm_http_router *rtr;

	rtr = malloc(sizeof(vtm_http_router));
	if (!rtr) {
		vtm_err_oom();
		return NULL;
	}

	rtr->routes = vtm_list_new(VTM_ELEM_POINTER, 8);
	if (!rtr->routes) {
		free(rtr);
		return NULL;
	}

	vtm_list_set_free_func(rtr->routes, vtm_http_router_http_free_rt);

	return rtr;
}

void vtm_http_router_free(vtm_http_router *rtr)
{
	if (!rtr)
		return;

	vtm_list_free(rtr->routes);
	free(rtr);
}

int vtm_http_router_add_rt(vtm_http_router *rtr, const char *path, struct vtm_http_route *rt)
{
	rt->url_path = vtm_str_copy(path);
	return vtm_list_add_va(rtr->routes, rt);
}

int vtm_http_router_static_rt(vtm_http_router *rtr, const char *path, vtm_http_static_rt_handle_fn fn)
{
	struct vtm_http_route *rt;

	rt = vtm_http_static_rt_new(path, fn);
	if (!rt)
		return vtm_err_get_code();

	return vtm_http_router_add_rt(rtr, path, rt);
}

int vtm_http_router_handle(vtm_http_router *rtr, struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res)
{
	int rc;
	struct vtm_http_route *rt;
	size_t i, rts_count;

	rts_count = vtm_list_size(rtr->routes);

	for (i=0; i < rts_count; i++) {
		rt = vtm_list_get_pointer(rtr->routes, i);

		if (!vtm_http_router_matches(req->path, rt->url_path))
			continue;

		rc = rt->fn_rt_handle(rt, ctx, req, res);
		if (rc != VTM_E_NOT_HANDLED)
			return rc;
	}

	return VTM_E_NOT_HANDLED;
}

static void vtm_http_router_http_free_rt(void *val)
{
	struct vtm_http_route *rt;

	rt = val;
	free(rt->url_path);
	rt->fn_rt_free(rt);
}

static bool vtm_http_router_matches(const char *path, const char *rt_path)
{
	int num;

	num = 0;
	while (*path == *rt_path) {
		if (*path == '\0' || *rt_path == '\0')
			break;

		path++;
		rt_path++;
		num++;
	}

	/* exact match */
	if (*path == *rt_path)
		return true;

	/* route path ends with / and matches path so far */
	if (num > 0 && *rt_path == '\0'	&& *(rt_path-1) == '/' && *(path-1) == '/')
		return true;

	return false;
}
