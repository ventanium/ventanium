/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file socket_writer.h
 *
 * @brief Buffered writing to a socket
 */

#ifndef VTM_NET_SOCKET_WRITER_H_
#define VTM_NET_SOCKET_WRITER_H_

#include <vtm/core/api.h>
#include <vtm/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vtm_socket_writer
{
	vtm_socket *sock;  /**< the socket were data is written to */
	char buf[1024];    /**< INTERNAL: write buffer */
	size_t index;      /**< INTERNAL: index in write buffer */
	int lrc;           /**< result code of last operation on writer */
};

/**
 * Resets buffer index and error state.
 *
 * @param sw the writer that should be reset
 */
VTM_API void vtm_socket_writer_reset(struct vtm_socket_writer *sw);

/**
 * Write memory chunk.
 *
 * @param sw the target writer
 * @param src the pointer to begin of memory chunk
 * @param len the length of the memory chunk
 * @return VTM_OK if the write was successful
 * @return VTM_ERROR or other error code if the call failed OR any previous
 *         call already failed and the writer was not reset after the error
 */
VTM_API int vtm_socket_writer_putm(struct vtm_socket_writer *sw, const char *src, size_t len);

/**
 * Write string.
 *
 * @param sw the target writer
 * @param str pointer to NUL-terminated string
 * @return VTM_OK if the write was successful
 * @return VTM_ERROR or other error code if the call failed OR any previous
 *         call already failed and the writer was not reset after the error
 */
VTM_API int vtm_socket_writer_puts(struct vtm_socket_writer *sw, const char *str);

/**
 * Write single character.
 *
 * @param sw the target writer
 * @param c character to write
 * @return VTM_OK if the write was successful
 * @return VTM_ERROR or other error code if the call failed OR any previous
 *         call already failed and the writer was not reset after the error
 */
VTM_API int vtm_socket_writer_putc(struct vtm_socket_writer *sw, char c);

/**
 * All data in buffer is written to socket.
 *
 * @param sw the target writer
 * @return VTM_OK if the data was successfully written to the socket
 * @return VTM_ERROR or other error code if the call failed OR any previous
 *         call already failed and the writer was not reset after the error
 */
VTM_API int vtm_socket_writer_flush(struct vtm_socket_writer *sw);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_SOCKET_WRITER_H_ */
