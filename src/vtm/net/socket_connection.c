/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "socket_connection.h"

#include <vtm/core/error.h>
#include <vtm/net/common.h>
#include <vtm/net/socket_intl.h>

static int vtm_socket_con_write_internal(struct vtm_socket_con *con);

struct vtm_socket_con* vtm_socket_con_new(vtm_socket *sock)
{
	struct vtm_socket_con *con;

	con = malloc(sizeof(*con));
	if (!con) {
		vtm_err_oom();
		return NULL;
	}

	if (vtm_socket_con_init(con, sock) != VTM_OK) {
		free(con);
		return NULL;
	}

	return con;
}

void vtm_socket_con_free(struct vtm_socket_con *con)
{
	vtm_socket_con_release(con);
	free(con);
}

int vtm_socket_con_init(struct vtm_socket_con *con, vtm_socket *sock)
{
	con->write_mtx = vtm_mutex_new();
	if (!con->write_mtx)
		return vtm_err_get_code();

	con->sock = sock;
	con->usr_data = NULL;
	con->writing = false;

	vtm_buf_init(&con->recvbuf, VTM_NET_BYTEORDER);
	vtm_buf_init(&con->sendbuf, VTM_NET_BYTEORDER);

	return VTM_OK;
}

void vtm_socket_con_release(struct vtm_socket_con *con)
{
	if (!con)
		return;

	vtm_mutex_free(con->write_mtx);
	vtm_buf_release(&con->recvbuf);
	vtm_buf_release(&con->sendbuf);
}

void vtm_socket_con_set_usr_data(struct vtm_socket_con *con, void *data)
{
	con->usr_data = data;
}

void* vtm_socket_con_get_usr_data(struct vtm_socket_con *con)
{
	return con->usr_data;
}

void vtm_socket_con_write_lock(struct vtm_socket_con *con)
{
	vtm_mutex_lock(con->write_mtx);
}

void vtm_socket_con_write_unlock(struct vtm_socket_con *con)
{
	vtm_mutex_unlock(con->write_mtx);
}

int vtm_socket_con_write_start(struct vtm_socket_con *con)
{
	if (con->writing)
		return VTM_OK;

	con->writing = true;
	return vtm_socket_con_write_internal(con);
}

int vtm_socket_con_write(struct vtm_socket_con *con)
{
	int rc;

	vtm_mutex_lock(con->write_mtx);
	rc = vtm_socket_con_write_internal(con);
	vtm_mutex_unlock(con->write_mtx);

	return rc;
}

static int vtm_socket_con_write_internal(struct vtm_socket_con *con)
{
	int rc;
	size_t written;

	rc = vtm_socket_write(con->sock, con->sendbuf.data + con->sendbuf.read,
		con->sendbuf.used - con->sendbuf.read, &written);

	con->sendbuf.read += written;
	vtm_buf_discard_processed(&con->sendbuf);

	if (rc == VTM_OK)
		con->writing = false;
	else
		vtm_socket_update_srv(con->sock);

	return rc;
}

int vtm_socket_con_close(struct vtm_socket_con *con)
{
	int rc;

	rc = vtm_socket_close(con->sock);
	if (rc == VTM_OK)
		vtm_socket_update_srv(con->sock);

	return rc;
}
