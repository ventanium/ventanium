/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "http_file_route.h"

#include <string.h> /* strlen() */
#include <vtm/core/error.h>
#include <vtm/core/string.h>
#include <vtm/fs/file.h>
#include <vtm/fs/path.h>
#include <vtm/net/http/http.h>
#include <vtm/net/http/http_error.h>
#include <vtm/net/http/http_file.h>
#include <vtm/net/http/http_request.h>
#include <vtm/net/http/http_response.h>

struct vtm_http_file_rt
{
	struct vtm_http_route    base;
	char                     *fs_real_root;
};

/* forward declaration */
static int vtm_http_file_rt_handle(struct vtm_http_route *rt, struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res);
static void vtm_http_file_rt_free(struct vtm_http_route *rt);
static char* vtm_http_file_rt_get_real_path(struct vtm_http_file_rt *rt, struct vtm_http_req *req);

struct vtm_http_route* vtm_http_file_rt_new(const char *fs_root)
{
	int rc;
	struct vtm_http_file_rt *rt;

	rt = malloc(sizeof(*rt));
	if (!rt) {
		vtm_err_oom();
		return NULL;
	}

	rt->base.fn_rt_handle = vtm_http_file_rt_handle;
	rt->base.fn_rt_free = vtm_http_file_rt_free;

	rc = vtm_path_get_real(fs_root, &(rt->fs_real_root));
	if (rc != VTM_OK) {
		vtm_http_file_rt_free(&rt->base);
		return NULL;
	}

	return &rt->base;
}

static int vtm_http_file_rt_handle(struct vtm_http_route *rt, struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res)
{
	int rc, attr;
	FILE *fp;
	char *filename;

	filename = vtm_http_file_rt_get_real_path((struct vtm_http_file_rt*) rt, req);
	if (!filename)
		return VTM_E_HTTP_NOT_FOUND;

	fp = fopen(filename, "r");
	if (!fp) {
		rc = VTM_E_HTTP_NOT_FOUND;
		goto cleanup;
	}

	rc = vtm_file_get_fattr(fp, &attr);
	if (rc != VTM_OK) {
		fclose(fp);
		goto cleanup;
	}

	if ((attr & VTM_FILE_ATTR_REG) == 0) {
		fclose(fp);
		rc = VTM_E_HTTP_NOT_FOUND;
		goto cleanup;
	}

	vtm_http_res_begin(res, VTM_HTTP_RES_MODE_FIXED, VTM_HTTP_200_OK);
	vtm_http_res_set_date(res);
	vtm_http_file_serve(res, filename, fp);
	vtm_http_res_end(res);

	rc = VTM_OK;

cleanup:
	free(filename);

	return rc;
}

static char* vtm_http_file_rt_get_real_path(struct vtm_http_file_rt *rt, struct vtm_http_req *req)
{
	int rc;
	char *rel_path;
	char *abs_path;
	size_t len;

	len = strlen(rt->fs_real_root) + (strlen(req->path) - strlen(rt->base.url_path) + 2);
	rel_path = malloc(len);
	if (!rel_path) {
		vtm_err_oom();
		return NULL;
	}

	strcpy(rel_path, rt->fs_real_root);
	strcat(rel_path, req->path + strlen(rt->base.url_path) - 1);

	rc = vtm_path_get_real(rel_path, &abs_path);
	if (rc != VTM_OK)
		goto out;

	if (!vtm_str_starts_with(abs_path, rt->fs_real_root)) {
		free(abs_path);
		abs_path = NULL;
		goto out;
	}

out:
	free(rel_path);

	return abs_path;
}

static void vtm_http_file_rt_free(struct vtm_http_route *rt)
{
	struct vtm_http_file_rt *frt;

	frt = (struct vtm_http_file_rt*) rt;

	free(frt->fs_real_root);
	free(rt);
}
