/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "http_upgrade.h"

#include <string.h> /* strcmp() */
#include <vtm/core/error.h>
#include <vtm/core/string.h>
#include <vtm/crypto/hash.h>
#include <vtm/util/base64.h>

#define VTM_HTTP_WS_VERSION         "13"
#define VTM_HTTP_WS_GUID            "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define VTM_HTTP_WS_MAX_KEY_LEN     128

bool vtm_http_is_ws_request(struct vtm_http_req *req)
{
	const char *field;

	field = vtm_http_req_get_header_str(req, VTM_HTTP_HEADER_CONNECTION);
	if (!vtm_str_list_contains(field, ",", VTM_HTTP_VALUE_UPGRADE, true))
		return false;

	field = vtm_http_req_get_header_str(req, VTM_HTTP_HEADER_UPGRADE);
	if (strcmp(field, VTM_HTTP_VALUE_WEBSOCKET) != 0)
		return false;

	field = vtm_http_req_get_header_str(req, VTM_HTTP_HEADER_SEC_WEBSOCKET_VERSION);
	if (strcmp(field, VTM_HTTP_WS_VERSION) != 0)
		return false;

	field = vtm_http_req_get_header_str(req, VTM_HTTP_HEADER_SEC_WEBSOCKET_KEY);
	if (field == NULL || strlen(field) == 0)
		return false;

	return true;
}

void vtm_http_get_ws_protocols(struct vtm_http_req *req, char ***protos, size_t *proto_count)
{
	const char *line;

	line = vtm_http_req_get_header_str(req, VTM_HTTP_HEADER_SEC_WEBSOCKET_PROTOCOL);
	if (!line)
		goto none;

	vtm_str_split_ex(line, ",", protos, proto_count);
	return;

none:
	protos = NULL;
	*proto_count = 0;
	return;
}

int vtm_http_upgrade_to_ws(struct vtm_http_req *req, vtm_http_res *res, char *proto)
{
	int rc;
	const char *req_key;
	char *hash_input;
	char *proto_copy;
	unsigned char hash[VTM_CRYPTO_SHA1_LEN];
	char base64[VTM_BASE64_ENC_BUF_LEN(VTM_CRYPTO_SHA1_LEN)];
	size_t req_key_len;
	size_t hash_input_len;

	rc = VTM_OK;
	hash_input = NULL;
	proto_copy = NULL;

	req_key = vtm_http_req_get_header_str(req, VTM_HTTP_HEADER_SEC_WEBSOCKET_KEY);
	if (!req_key)
		return VTM_ERROR;

	req_key_len = strlen(req_key);
	if (req_key_len > VTM_HTTP_WS_MAX_KEY_LEN)
		return VTM_ERROR;

	hash_input_len = req_key_len + strlen(VTM_HTTP_WS_GUID);
	hash_input = malloc(hash_input_len + 1);
	if (!hash_input)
		return VTM_ERROR;

	hash_input[0] = '\0';
	strcat(hash_input, req_key);
	strcat(hash_input, VTM_HTTP_WS_GUID);
	rc = vtm_crypto_sha1(hash_input, hash_input_len, hash, sizeof(hash));
	if (rc != VTM_OK)
		goto cleanup;

	rc = vtm_base64_encode(hash, sizeof(hash), base64, sizeof(base64));
	if (rc != VTM_OK)
		goto cleanup;

	vtm_http_res_begin(res, VTM_HTTP_RES_MODE_FIXED, VTM_HTTP_101_SWITCHING_PROTOCOLS);
	vtm_http_res_header(res, VTM_HTTP_HEADER_CONNECTION, VTM_HTTP_VALUE_UPGRADE);
	vtm_http_res_header(res, VTM_HTTP_HEADER_UPGRADE, VTM_HTTP_VALUE_WEBSOCKET);
	vtm_http_res_header(res, VTM_HTTP_HEADER_SEC_WEBSOCKET_ACCEPT, base64);

	if (proto) {
		proto_copy = vtm_str_copy(proto);
		vtm_http_res_header(res, VTM_HTTP_HEADER_SEC_WEBSOCKET_PROTOCOL, proto);
	}

	vtm_http_res_set_action(res, VTM_HTTP_RES_ACT_UPGRADE_WS, proto_copy);
	vtm_http_res_end(res);

cleanup:
	free(hash_input);
	return rc;
}
