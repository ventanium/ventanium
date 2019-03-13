/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "http_util.h"

#include <string.h> /* strcpy(), strcat(), strncat() */
#include <vtm/core/error.h>
#include <vtm/core/string.h>
#include <vtm/util/base64.h>

int vtm_http_basic_auth_require(vtm_http_res *res, const char *realm)
{
	char buf[128];

	strcpy(buf, "Basic realm=\"");
	strncat(buf, realm, sizeof(buf) - strlen(buf) - 2);
	strcat(buf, "\"");

	return vtm_http_res_header(res, VTM_HTTP_HEADER_WWW_AUTHENTICATE, buf);
}

int vtm_http_basic_auth_read(struct vtm_http_req *req, char **user, char **pass)
{
	int rc;
	const char *field;
	char *decoded, **tokens;
	size_t decoded_len, token_count;

	field = vtm_http_req_get_header_str(req, VTM_HTTP_HEADER_AUTHORIZATION);
	if (!field)
		return VTM_ERROR;

	rc = strncmp("Basic ", field, strlen("Basic "));
	if (rc != 0)
		return VTM_ERROR;

	field += strlen("Basic ");

	decoded_len = VTM_BASE64_DEC_BUF_LEN(strlen(field));
	decoded = malloc(decoded_len);
	if (!decoded)
		return VTM_ERROR;

	rc = vtm_base64_decode(field, strlen(field), decoded, &decoded_len);
	if (rc != VTM_OK) {
		free(decoded);
		return VTM_ERROR;
	}

	decoded[decoded_len] = '\0';
	rc = vtm_str_split_ex(decoded, ":", &tokens, &token_count);
	free(decoded);
	if (rc != VTM_OK)
		return rc;

	if (token_count != 2) {
		rc = vtm_err_sets(VTM_ERROR, "Wrong auth format");
		goto end;
	}

	*user = vtm_str_copy(tokens[0]);
	*pass = vtm_str_copy(tokens[1]);

end:
	free(tokens);
	return rc;
}
