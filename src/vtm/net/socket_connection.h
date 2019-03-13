/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file socket_connection.h
 *
 * @brief Stream socket connection
 */

#ifndef VTM_NET_SOCKET_CONNECTION_H_
#define VTM_NET_SOCKET_CONNECTION_H_

#include <vtm/core/api.h>
#include <vtm/core/buffer.h>
#include <vtm/core/types.h>
#include <vtm/net/socket.h>
#include <vtm/util/mutex.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vtm_socket_con
{
	vtm_socket *sock;

	struct vtm_buf recvbuf;
	struct vtm_buf sendbuf;
	void *usr_data;

	vtm_mutex *write_mtx;
	bool writing;
};

/**
 * Creates and initializes a new socket connection on the heap.
 *
 * @param sock the socket that should be associated with the connection
 * @return the created connection
 * @return NULL if memory allocation or initialization failed
 */
VTM_API struct vtm_socket_con* vtm_socket_con_new(vtm_socket *sock);

/**
 * Releases a socket connection and frees the pointer.
 *
 * After this call the connection pointer is no longer valid.
 *
 * @param con the connection that should be released
 */
VTM_API void vtm_socket_con_free(struct vtm_socket_con *con);

/**
 * Initializes a new socket connection.
 *
 * @return VTM_OK if the initialization was successful
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_con_init(struct vtm_socket_con *con, vtm_socket *sock);

/**
 * Releases a socket connection.
 *
 * After this call the connection structure is no longer valid.
 *
 * @param con the connection that should be released
 */
VTM_API void vtm_socket_con_release(struct vtm_socket_con *con);

/**
 * Stores arbitrary user data with the connection.
 *
 * @param con the connection that should store the user data
 * @param data the data that should be stored
 */
VTM_API void vtm_socket_con_set_usr_data(struct vtm_socket_con *con, void *data);

/**
 * Retrieves the stored user data.
 *
 * @param con the connection
 * @return the previous saved user data
 * @return NULL if no data was stored
 */
VTM_API void* vtm_socket_con_get_usr_data(struct vtm_socket_con *con);

/**
 * Locks the internal writer buffer.
 *
 * This method must be called if new data should be stored
 * in the output buffer of the connection.
 *
 * @param con the connection that should be locked
 */
VTM_API void vtm_socket_con_write_lock(struct vtm_socket_con *con);

/**
 * Unlocks the internal writer buffer.
 *
 * This method must be called after new data was tored
 * in the output buffer of the connection.
 *
 * @param con the connection that should be unlocked
 */
VTM_API void vtm_socket_con_write_unlock(struct vtm_socket_con *con);

/**
 * Starts sending the contents of the output buffer.
 *
 * vtm_socket_con_write_lock() must be called prior to this function.
 *
 * @param con the connection that should send data
 * @return VTM_OK if the call succeed
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_con_write_start(struct vtm_socket_con *con);

/**
 * Sends the next chunk of data from the output buffer
 *
 * @param con the connection that should send data
 * @return VTM_OK if all remaining data was sent
 * @return VTM_E_IO_AGAIN if the function must be called again later
 * @return VTM_E_IO_CLOSED if the connection was closed
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_con_write(struct vtm_socket_con *con);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_SOCKET_CONNECTION_H_ */
