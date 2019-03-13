/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/net/socket.h>

#include <vtm/core/error.h>

vtm_socket* vtm_socket_tls_new(enum vtm_socket_family fam, struct vtm_socket_tls_opts *opts)
{
	vtm_err_set(VTM_E_NOT_SUPPORTED);
	return NULL;
}
