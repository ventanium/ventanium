/*
 * Copyright (C) 2018-2023 Matthias Benkendorf
 */

/**
 * @file nm_stream_connection.h
 *
 * @brief Client connection of network message server
 */

#ifndef VTM_NET_NM_NM_STREAM_CONNECTION_H_
#define VTM_NET_NM_NM_STREAM_CONNECTION_H_

#include <vtm/core/api.h>
#include <vtm/core/dataset.h>

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

/**
 * Sets option on underlying socket
 *
 * @param con the connection
 * @param opt the option that should be set
 * @param val the value that should be set
 * @param len length of the value in bytes
 * @return VTM_OK if the option was successfully set
 * @return VTM_E_NOT_SUPPORTED if the option is not supported
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_nm_stream_con_set_opt(vtm_nm_stream_con *con, int opt, const void *val, size_t len);

/**
 * Retrieves current option value from underlying socket
 *
 * @param con the connection
 * @param opt the option that should be read
 * @param[out] val the current value of the option
 * @param len length of value in bytes
 * @return VTM_OK if the option was successfully read
 * @return VTM_E_NOT_SUPPORTED if the option is not supported
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_nm_stream_con_get_opt(vtm_nm_stream_con *con, int opt, void *val, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_NM_NM_STREAM_CONNECTION_H_ */
