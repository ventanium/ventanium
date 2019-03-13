/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file crypto/hash.h
 *
 * @brief Cryptographic hash functions
 */

#ifndef VTM_CRYPTO_HASH_H_
#define VTM_CRYPTO_HASH_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/crypto/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VTM_CRYPTO_SHA1_LEN		20  /**< length of SHA1 hash in bytes */

/**
 * Builts the SHA1 hash over a given memory region.
 *
 * @param in pointer to start of memory region
 * @param len the size of the memory region
 * @param buf pointer to buffer where hash will be written to
 * @param buf_len size of supplied buffer
 * @return VTM_OK if the hash was calculated successfully
 * @return VTM_E_INVALID_ARG if one of the arguments was not valid, for example
 *         if a too small buffer was supplied
 * @return VTM_ERROR if the hash could not be calculated
 */
VTM_API int vtm_crypto_sha1(const void *in, size_t len, void *buf, size_t buf_len);

#ifdef __cplusplus
}
#endif

#endif /* VTM_CRYPTO_HASH_H_ */
