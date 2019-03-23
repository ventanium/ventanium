/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file crypto/random.h
 *
 * @brief Cryptographically secure pseudo-random numbers
 */

#ifndef VTM_CRYPTO_RANDOM_H_
#define VTM_CRYPTO_RANDOM_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/crypto/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generates cryptographically secure pseudo-random bytes.
 *
 * @param[out] buf holds the generated random bytes
 * @param len the size of the output buffer in bytes
 * @return VTM_OK if the buffer was successfully filled with random bytes
 * @return VTM_E_NOT_SUPPORTED if library was built without crypto support
 * @return VTM_ERROR if an error occured, for example if underlaying PRNG has
 *         not enough randomness
 */
VTM_API int vtm_crypto_random_bytes(void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* VTM_CRYPTO_RANDOM_H_ */
