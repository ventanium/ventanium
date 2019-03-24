/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file blob.h
 *
 * @brief Blob data type
 */

#ifndef VTM_CORE_BLOB_H_
#define VTM_CORE_BLOB_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocates a new blob.
 *
 * @param len the size of the blob in bytes
 * @return the allocated blob
 * @return NULL if the allocation failed
 */
VTM_API void* vtm_blob_new(size_t len);

/**
 * Determines the size of a blob.
 *
 * @param blob the blob whose size should be determined
 * @return the size of the blob in bytes
 */
VTM_API size_t vtm_blob_size(const void *blob);

/**
 * Releases the blob memory.
 *
 * After this call the blob pointer is no longer valid.
 *
 * @param blob the blob that should be released
 */
VTM_API void vtm_blob_free(void *blob);

#ifdef __cplusplus
}
#endif

#endif /* VTM_CORE_BLOB_H_ */
