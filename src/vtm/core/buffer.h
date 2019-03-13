/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file buffer.h
 *
 * @brief Automatically growing buffer
 */

#ifndef VTM_CORE_BUFFER_H_
#define VTM_CORE_BUFFER_H_

#include <vtm/core/api.h>
#include <vtm/core/system.h>
#include <vtm/core/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Checks if buffer has NUM unprocessed bytes available */
#define VTM_BUF_GET_AVAIL(BUF_PTR, NUM)            \
	((BUF_PTR)->read + NUM <= (BUF_PTR)->used &&   \
	 (BUF_PTR)->read + NUM >= (BUF_PTR)->read)

/** Gets the total number of unprocessed bytes */
#define VTM_BUF_GET_AVAIL_TOTAL(BUF_PTR)           \
	((BUF_PTR)->used - (BUF_PTR)->read)

/** Reads one character from the buffer */
#define VTM_BUF_GETC(BUF_PTR)                      \
	((BUF_PTR)->data[(BUF_PTR)->read++])

/** Checks if buffer has space for additional NUM bytes */
#define VTM_BUF_PUT_AVAIL(BUF_PTR, NUM)            \
	((BUF_PTR)->used + NUM <= (BUF_PTR)->len &&    \
	 (BUF_PTR)->used + NUM >= (BUF_PTR)->used)

/** Space left in the buffer in bytes */
#define VTM_BUF_PUT_AVAIL_TOTAL(BUF_PTR)           \
	((BUF_PTR)->len - (BUF_PTR)->used)

/** Get pointer to begin of unused memory */
#define VTM_BUF_PUT_PTR(BUF_PTR)                   \
	((BUF_PTR)->data + (BUF_PTR)->used)

/** Increases the number of used bytes */
#define VTM_BUF_PUT_INC(BUF_PTR, NUM)              \
	((BUF_PTR)->used += NUM)

/** Marks all bytes in the buffer as processed */
#define VTM_BUF_PROCESS_ALL(BUF_PTR)               \
	(BUF_PTR)->read = (BUF_PTR)->used

struct vtm_buf
{
	enum vtm_byteorder  order;         /**< the byte order of the buffer */
	unsigned char       *data;         /**< pointer to begin of buffer memory */
	unsigned char       sdata[1024];   /**< static memory for small sizes */
	size_t              len;           /**< current size of buffer in bytes */
	size_t              used;          /**< used bytes */
	size_t              read;          /**< read bytes */
	int                 err;           /**< last error code */
};

/**
 * Creates a new buffer on the heap.
 *
 * @param order the byteorder of the buffer
 * @return pointer to created buffer
 * @return NULL if an error occured
 */
VTM_API struct vtm_buf* vtm_buf_new(enum vtm_byteorder order);

/**
 * Initializes given buffer.
 *
 * @param buf the buffer that should be initialized
 * @param order the byteorder of the buffer
 */
VTM_API void vtm_buf_init(struct vtm_buf *buf, enum vtm_byteorder order);

/**
 * Releases the buffer and all allocated resources
 *
 * @param buf the buffer that should be released
 */
VTM_API void vtm_buf_release(struct vtm_buf *buf);

/**
 * Releases the buffer and frees the pointer to it.
 *
 * After this call the buffer pointer is no longer valid.
 *
 * @param buf the buffer that should be freed
 */
VTM_API void vtm_buf_free(struct vtm_buf *buf);

/**
 * Resets the buffer so that there are no more used or processed bytes.
 *
 * @param buf the buffer that should be cleared
 */
VTM_API void vtm_buf_clear(struct vtm_buf *buf);

/**
 * Ensures that the buffer has enough free space to store the given
 * number of bytes.
 *
 * @param buf the buffer
 * @param space the number of bytes
 * @return VTM_OK if the buffer has enough free space
 * @return VTM_E_MALLOC or VTM_ERROR if an error occured
 */
VTM_API int vtm_buf_ensure(struct vtm_buf *buf, size_t space);

/**
 * Removes already processed bytes from the buffer.
 *
 * @param buf the buffer that should discard processed bytes
 */
VTM_API void vtm_buf_discard_processed(struct vtm_buf *buf);

/**
 * Marks the given number of bytes as processed.
 *
 * @param buf the buffer where the bytes should be marked
 * @param num the number of bytes that should be marked
 * @return VTM_OK if the bytes were successfully marked
 * @return VTM_E_OVERFLOW if the number of processed bytes would overflow
 */
VTM_API int vtm_buf_mark_processed(struct vtm_buf *buf, size_t num);

/**
 * Copies given amount of bytes from the buffer to destination.
 *
 * If necessary the data is transformed to match the destination
 * byte order.
 *
 * @param buf the buffer where the data should be read from
 * @param dst the target buffer where the data is copied to
 * @param len the number of bytes that should be copied
 * @param dst_order the output byteorder
 * @return VTM_OK if the bytes were read successfully
 * @return VTM_ERROR if any error has already occured in a previous call and
 *         the buffer was not reset
 */
VTM_API int vtm_buf_geto(struct vtm_buf *buf, void *dst, size_t len, enum vtm_byteorder dst_order);

/**
 * Copies given amount of bytes from the buffer to destination.
 *
 * @param buf the buffer where the data should be read from
 * @param dst the target buffer where the data is copied to
 * @param len the number of bytes that should be copied
 * @return VTM_OK if the bytes were read successfully
 * @return VTM_ERROR if any error has already occured in a previous call and
 *         the buffer was not reset
 */
VTM_API int vtm_buf_getm(struct vtm_buf *buf, void *dst, size_t len);

/**
 * Stores given amount of bytes in the buffer.
 *
 * If necessary the data is transformed to match the byte order of the buffer.
 *
 * @param buf the buffer where the data should be stored
 * @param src the source where the data is copied from
 * @param len the number of bytes that should be copied
 * @param src_order the input byteorder
 * @return VTM_OK if the bytes were stored successfully
 * @return VTM_ERROR if any error has already occured in a previous call and
 *         the buffer was not reset
 */
VTM_API int vtm_buf_puto(struct vtm_buf *buf, const void *src, size_t len, enum vtm_byteorder src_order);

/**
 * Stores given amount of bytes in the buffer.
 *
 * @param buf the buffer where the data should be stored
 * @param src the source where the data is copied from
 * @param len the number of bytes that should be copied
 * @return VTM_OK if the bytes were stored successfully
 * @return VTM_ERROR if any error has already occured in a previous call and
 *         the buffer was not reset
 */
VTM_API int vtm_buf_putm(struct vtm_buf *buf, const void *src, size_t len);

/**
 * Stores given string in the buffer.
 *
 * @param buf the buffer where the string should be stored
 * @param str the source string that is copied to the buffer
 * @return VTM_OK if the string was stored successfully
 * @return VTM_ERROR if any error has already occured in a previous call and
 *         the buffer was not reset
 */
VTM_API int vtm_buf_puts(struct vtm_buf *buf, const char *str);

/**
 * Stores given character in the buffer.
 *
 * @param buf the buffer where the character should be stored
 * @param c the character that should be stored
 * @return VTM_OK if the bytes were stored successfully
 * @return VTM_ERROR if any error has already occured in a previous call and
 *         the buffer was not reset
 */
VTM_API int vtm_buf_putc(struct vtm_buf *buf, unsigned char c);

#ifdef __cplusplus
}
#endif

#endif /* VTM_CORE_BUFFER_H_ */
