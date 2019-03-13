/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file http_memory.h
 *
 * @brief Global server memory
 */

#ifndef VTM_NET_HTTP_HTTP_MEMORY_H_
#define VTM_NET_HTTP_HTTP_MEMORY_H_

#include <vtm/core/api.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtm_http_mem vtm_http_mem;

/**
 * Stores a single value in the memory.
 *
 * @param mem the global memory
 * @param key the key under which the value is stored
 * @param val the value that is stored
 */
VTM_API void vtm_http_mem_put(vtm_http_mem *mem, const char *key, void *val);

/**
 * Retrieves a stored value from the memory.
 *
 * @param mem the global memory
 * @param key the key for the value that should be retrieved
 */
VTM_API void* vtm_http_mem_get(vtm_http_mem *mem, const char *key);

/**
 * Acquire global lock for given key.
 *
 * This call may block until the lock can be acquired.
 *
 * @param mem the global memory
 * @param key the key that should be locked
 */
VTM_API void vtm_http_mem_lock(vtm_http_mem *mem, const char *key);

/**
 * Unlock given key.
 *
 * @param mem the global memory
 * @param key the key that should be unlocked
 */
VTM_API void vtm_http_mem_unlock(vtm_http_mem *mem, const char *key);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_MEMORY_H_ */
