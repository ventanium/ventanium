/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file socket_emitter.h
 *
 * @brief Nonblocking data transmission
 */

#ifndef VTM_NET_SOCKET_EMITTER_H_
#define VTM_NET_SOCKET_EMITTER_H_

#include <stdio.h> /* FILE */
#include <vtm/core/api.h>
#include <vtm/core/buffer.h>
#include <vtm/core/types.h>
#include <vtm/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

enum vtm_socket_emitter_result
{
	VTM_SOCK_EMIT_AGAIN,     /**< There is still data that needs to be transmitted */
	VTM_SOCK_EMIT_COMPLETE,  /**< All data was transmitted */
	VTM_SOCK_EMIT_ERROR      /**< An error occured while transmitting data */
};

struct vtm_socket_emitter
{
	/** the socket that is used by the transmitter */
	vtm_socket *sock;

	/** length of data in bytes */
	uint64_t length;

	/** function that is called when the emitter should write data */
	enum vtm_socket_emitter_result (*vtm_sock_emt_write)(struct vtm_socket_emitter *se);

	/** function that is called when the emitter is released */
	void (*vtm_sock_emt_clean)(struct vtm_socket_emitter *se);

	/** pointer to next emitter in chain */
	struct vtm_socket_emitter *next;
};

/**
 * Releases the given emitter and all other linked emitters in
 * the chain.
 *
 * @param se the socket emitter that should be released
 */
VTM_API void vtm_socket_emitter_free_chain(struct vtm_socket_emitter *se);

/**
 * Releases the given emitter.
 *
 * @param se the socket emitter that should be released
 */
VTM_API void vtm_socket_emitter_free_single(struct vtm_socket_emitter *se);

/**
 * Creates a new socket emitter for sending a raw memory chunk.
 *
 * @param sock the socket that should be used by the emitter
 * @param src pointer to the memory chunk
 * @param len the size of the memory chunk in bytes
 * @return the created emitter
 * @return NULL if an error occured
 */
VTM_API struct vtm_socket_emitter* vtm_socket_emitter_for_raw(vtm_socket *sock, const void *src, size_t len);

/**
 * Creates a new socket emitter for sending the contents of buffer.
 *
 * @param sock the socket that should be used by the emitter
 * @param buf the buffer whose contents should be sent
 * @param fr if true the buffer is released when the emitter is released
 * @return the created emitter
 * @return NULL if an error occured
 */
VTM_API struct vtm_socket_emitter* vtm_socket_emitter_for_buffer(vtm_socket *sock, struct vtm_buf *buf, bool fr);

/**
 * Creates a new socket emitter for sending the contents of file.
 *
 * @param sock the socket that should be used by the emitter
 * @param fp the already opened file
 * @return the created emitter
 * @return NULL if an error occured
 */
VTM_API struct vtm_socket_emitter* vtm_socket_emitter_for_file(vtm_socket *sock, FILE *fp);

/**
 * Tries to send the data of all emitters in the chain immediately.
 *
 * Emitters that were able to send their data completely are automatically
 * released.
 *
 * @param[in, out] se pointer to pointer to root emitter.
 * @return VTM_OK if the data of all emitters was transmitted successfully
 * @return VTM_E_IO_AGAIN if only some data was transmitted. In this case the
 *         se parameter points to the emitter that is now the head of the
 *         chain.
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_emitter_try_write(struct vtm_socket_emitter **se);

/**
 * Calculates the sum of all length specifications of the emitter chain.
 *
 * @param se the root emitter
 * @param[out] out_sum the total number in bytes
 * @return VTM_OK if the sum was calculated successfully
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_emitter_get_chain_lensum(struct vtm_socket_emitter *se, uint64_t *out_sum);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_SOCKET_EMITTER_H_ */
