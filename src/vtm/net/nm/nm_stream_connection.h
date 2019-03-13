/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file nm_stream_connection.h
 *
 * @brief Client connection of network message server
 */

#ifndef VTM_NET_NM_NM_STREAM_CONNECTION_H_
#define VTM_NET_NM_NM_STREAM_CONNECTION_H_

#include <vtm/core/api.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtm_nm_stream_con vtm_nm_stream_con;

/**
 * Stores arbitrary user data with the connection.
 *
 * @param con the connection that should store the user data
 * @param data the data that should be stored
 */
VTM_API void vtm_nm_stream_con_set_usr_data(vtm_nm_stream_con *con, void *data);

/**
 * Retrieves the stored user data.
 *
 * @param con the connection
 * @return the previous saved user data
 * @return NULL if no data was stored
 */
VTM_API void* vtm_nm_stream_con_get_usr_data(vtm_nm_stream_con *con);

/**
 * Sends a message to the client.
 *
 * @param con the connection
 * @param msg the message that sould be sent
 * @return VTM_OK if the message was sent successfully
 * @return VTM_E_IO_UNKOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_nm_stream_con_send(vtm_nm_stream_con *con, vtm_dataset *msg);

/**
 * Closes the client connection gracefully.
 *
 * @param con the connection that should be closed
 */
VTM_API void vtm_nm_stream_con_close(vtm_nm_stream_con *con);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_NM_NM_STREAM_CONNECTION_H_ */
