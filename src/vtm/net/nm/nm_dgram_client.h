/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file nm_dgram_client.h
 *
 * @brief Datagram based network message client
 */

#ifndef VTM_NET_NM_NM_DGRAM_CLIENT_H_
#define VTM_NET_NM_NM_DGRAM_CLIENT_H_

#include <vtm/core/api.h>
#include <vtm/core/dataset.h>
#include <vtm/net/network.h>
#include <vtm/net/socket_addr.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VTM_NM_DGRAM_CL_OPT_RECV_TIMEOUT    1  /**< expects unsigned long, value is millisceonds */

typedef struct vtm_nm_dgram_client vtm_nm_dgram_client;

/**
 * Creates a new client.
 *
 * @return the created client which can be used in the other functions
 * @return NULL if an error occured
 */
VTM_API vtm_nm_dgram_client* vtm_nm_dgram_client_new(enum vtm_socket_family fam);

/**
 * Releases the client and all allocated resources.
 *
 * After this call the client pointer is no longer valid.
 *
 * @param cl the client that should be released
 */
VTM_API void vtm_nm_dgram_client_free(vtm_nm_dgram_client *cl);

/**
 * Sets one of the possible options.
 *
 * The possible options are macros starting with VTM_NM_DGRAM_CL_OPT_.
 *
 * @param cl the client where the option should be set
 * @param opt the option that should be set
 * @param val pointer to new value of the option
 * @param len size of the value
 * @return VTM_OK if the option was successfully set
 * @return VTM_E_NOT_SUPPORTED if the given option or the value format is
 *         not supported
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_nm_dgram_client_set_opt(vtm_nm_dgram_client *cl, int opt, const void *val, size_t len);

/**
 * Sends a message to given destination adress.
 *
 * @param cl the client
 * @param msg the dataset containing the message values
 * @param saddr the destination adress
 * @return VTM_OK if the message was successfully sent
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_nm_dgram_client_send(vtm_nm_dgram_client *cl, vtm_dataset *msg, struct vtm_socket_saddr *saddr);

/**
 * Tries to receive a message.
 *
 * @param cl the client
 * @param[out] msg the dataset where the received message is stored
 * @param[out] saddr if not NULL the source adress is stored here
 * @return VTM_OK if a message was successfully received
 * @return VTM_E_TIMEOUT if the timeout elapsed and no message was received
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_nm_dgram_client_recv(vtm_nm_dgram_client *cl, vtm_dataset *msg, struct vtm_socket_saddr *saddr);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_NM_NM_DGRAM_CLIENT_H_ */
