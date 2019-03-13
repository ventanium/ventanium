/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "http_request.h"

#include <vtm/core/error.h>
#include <vtm/net/http/http_connection_base_intl.h>
#include <vtm/net/http/http_request_intl.h>

void vtm_http_req_release(struct vtm_http_req *req)
{
	vtm_dataset_free(req->headers);
	vtm_dataset_free(req->params);
}

const char* vtm_http_req_get_host(struct vtm_http_req *req)
{
	return vtm_http_req_get_header_str(req, VTM_HTTP_HEADER_HOST);
}

const char* vtm_http_req_get_header_str(struct vtm_http_req *req, const char *name)
{
	if (!req->headers)
		return NULL;

	return vtm_dataset_get_string(req->headers, name);
}

const char* vtm_http_req_get_query_str(struct vtm_http_req *req, const char *name)
{
	if (req->params)
		return vtm_dataset_get_string(req->params, name);

	return NULL;
}

int vtm_http_req_get_remote_info(struct vtm_http_req *req, char *buf, size_t len, unsigned int *port)
{
	int rc;
	struct vtm_http_con_base *con;
	struct vtm_socket_saddr saddr;

	con = req->con;
	if (!con)
		return VTM_E_INVALID_STATE;

	rc = vtm_socket_get_remote_addr(con->sock, &saddr);
	if (rc != VTM_OK)
		return rc;

	return vtm_socket_os_addr_convert(&saddr, NULL, buf, len, port);
}
