/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "hash.h"

#include <stdlib.h> /* malloc() */
#include <vtm/core/error.h>

#ifdef VTM_LIB_OPENSSL

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

#else /* no crypto library supported */

int vtm_crypto_sha1(const void *in, size_t len, void *buf, size_t buf_len)
{
	return vtm_err_set(VTM_E_NOT_SUPPORTED);
}

#endif 
