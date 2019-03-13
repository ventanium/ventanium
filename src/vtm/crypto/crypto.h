/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file crypto.h
 *
 * @brief Crypto module basics
 */

#ifndef VTM_CRYPTO_CRYPTO_H_
#define VTM_CRYPTO_CRYPTO_H_

#include <vtm/core/api.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the crypto module.
 *
 * This function must be called before you can use other functionality
 * that depend on cryptography like TLS Sockets.
 *
 * @return VTM_OK if initialization was successful or is not needed on current
 *                platform
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_module_crypto_init(void);

/**
 * Deinitializes the crypto module.
 *
 * All allocated resources are released.
 */
VTM_API void vtm_module_crypto_end(void);

#ifdef __cplusplus
}
#endif

#endif /* VTM_CRYPTO_CRYPTO_H_ */
