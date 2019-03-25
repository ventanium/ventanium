/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "hash.h"

#include <stdlib.h> /* malloc() */
#include <vtm/core/error.h>

#ifdef VTM_LIB_OPENSSL

#include <openssl/evp.h>
#include <openssl/sha.h>

int vtm_crypto_sha1(const void *in, size_t len, void *buf, size_t buf_len)
{
	unsigned char *check;

	if (!in || !buf || buf_len < VTM_CRYPTO_SHA1_LEN)
		return VTM_E_INVALID_ARG;

	check = SHA1((unsigned char*) in, len, buf);
	if (!check)
		return VTM_ERROR;

	return VTM_OK;
}

int vtm_crypto_hmac_sha512(const void *key, size_t key_len,
	const void *salt, size_t salt_len,
	void *buf, size_t buf_len, uint32_t iterations)
{
	int rc;

#if OPENSSL_VERSION_NUMBER >= 0x10000000L
	rc = PKCS5_PBKDF2_HMAC(key, key_len, salt, salt_len, iterations, EVP_sha512(), buf_len, buf);
	rc = (rc == 1) ? VTM_OK : VTM_ERROR;
#else
	rc = VTM_E_NOT_SUPPORTED;
#endif

	return rc;
}

#else /* no crypto library supported */

int vtm_crypto_sha1(const void *in, size_t len, void *buf, size_t buf_len)
{
	return vtm_err_set(VTM_E_NOT_SUPPORTED);
}

int vtm_crypto_hmac_sha512(const void *key, size_t key_len,
	const void *salt, size_t salt_len,
	void *buf, size_t buf_len, uint32_t iterations)
{
	return vtm_err_set(VTM_E_NOT_SUPPORTED);
}

#endif
