/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "http_file.h"

#include <vtm/core/error.h>
#include <vtm/fs/mime.h>
#include <vtm/net/socket_emitter.h>
#include <vtm/net/http/http.h>
#include <vtm/net/http/http_response.h>

int vtm_http_file_serve(vtm_http_res *res, const char *name, FILE *fp)
{
	int rc;
	struct vtm_socket_emitter *se;
	const char *mimetype;

	/* content type */
	mimetype = vtm_mime_type_for_name(name);
	if (mimetype) {
		rc = vtm_http_res_header(res, VTM_HTTP_HEADER_CONTENT_TYPE, mimetype);
		if (rc != VTM_OK)
			return rc;
	}

	/* emitter */
	se = vtm_socket_emitter_for_file(NULL, fp);
	if (!se)
		return vtm_err_get_code();

	return vtm_http_res_body_emt(res, se);
}
