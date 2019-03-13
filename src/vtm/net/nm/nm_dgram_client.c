/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "nm_dgram_client.h"

#include <vtm/core/error.h>
#include <vtm/net/common.h>
#include <vtm/net/socket.h>
#include <vtm/net/nm/nm_protocol_intl.h>

struct vtm_nm_dgram_client
{
	vtm_socket *sock;
};

vtm_nm_dgram_client* vtm_nm_dgram_client_new(enum vtm_socket_family fam)
{
	vtm_nm_dgram_client *cl;

	cl = malloc(sizeof(*cl));
	if (!cl) {
		vtm_err_oom();
		return NULL;
	}

	cl->sock = vtm_socket_new(fam, VTM_SOCK_TYPE_DGRAM);
	if (!cl->sock) {
		free(cl);
		return NULL;
	}

	return cl;
}

void vtm_nm_dgram_client_free(vtm_nm_dgram_client *cl)
{
	if (!cl)
		return;

	vtm_socket_free(cl->sock);
	free(cl);
}

int vtm_nm_dgram_client_set_opt(vtm_nm_dgram_client *cl, int opt, const void *val, size_t len)
{
	switch (opt) {
		case VTM_NM_DGRAM_CL_OPT_RECV_TIMEOUT:
			return vtm_socket_set_opt(cl->sock, VTM_SOCK_OPT_RECV_TIMEOUT, val, len);

		default:
			break;
	}

	return VTM_E_NOT_SUPPORTED;
}

int vtm_nm_dgram_client_send(vtm_nm_dgram_client *cl, vtm_dataset *msg, struct vtm_socket_saddr *saddr)
{
	int rc;
	struct vtm_buf buf;
	size_t sent;

	vtm_buf_init(&buf, VTM_NET_BYTEORDER);

	rc = vtm_nm_msg_to_buf(msg, &buf);
	if (rc != VTM_OK)
		goto end;

	rc = vtm_socket_dgram_send(cl->sock, buf.data, buf.used, &sent, saddr);
	if (rc != VTM_OK)
		goto end;

	if (buf.used != sent)
		rc = vtm_err_set(VTM_E_IO_PARTIAL);

end:
	vtm_buf_release(&buf);

	return rc;
}

int vtm_nm_dgram_client_recv(vtm_nm_dgram_client *cl, vtm_dataset *msg, struct vtm_socket_saddr *saddr)
{
	int rc;
	struct vtm_buf buf;

	vtm_buf_init(&buf, VTM_NET_BYTEORDER);

	rc = vtm_socket_dgram_recv(cl->sock, buf.data, buf.len, &buf.used, saddr);
	if (rc != VTM_OK)
		goto end;

	rc = vtm_nm_msg_from_buf(msg, &buf);

end:
	vtm_buf_release(&buf);

	return rc;
}
