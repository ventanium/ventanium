/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file nm_protocol.h
 *
 * @brief Network message serialization functions
 */

#ifndef VTM_NET_NM_NM_PROTOCOL_H_
#define VTM_NET_NM_NM_PROTOCOL_H_

#include <vtm/core/api.h>
#include <vtm/core/buffer.h>
#include <vtm/core/dataset.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stores the contents of the dataset as binary data in the buffer.
 *
 * @param msg the message that should be serialized
 * @param[out] buf holds the serialized data of the message
 * @return VTM_OK if the serialization was successful
 * @return VTM_E_NOT_SUPPORTED if the message contains data that could not be
 *         serialized
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_nm_msg_to_buf(vtm_dataset *msg, struct vtm_buf *buf);

/**
 * Rebuilds the original message from the contents of a buffer.
 *
 * @param[out] msg holds the message
 * @param buf the buffer where the binary serialzed data is read from
 * @return VTM_OK if the deserialization was successful
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_nm_msg_from_buf(vtm_dataset *msg, struct vtm_buf *buf);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_NM_NM_PROTOCOL_H_ */
