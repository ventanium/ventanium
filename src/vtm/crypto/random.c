/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "random.h"

#include <vtm/core/error.h>

#ifdef VTM_LIB_OPENSSL

#include <openssl/rand.h>

int vtm_crypto_random_bytes(void *buf, size_t len)
{
	int rc;

	if (len > INT_MAX)
		return VTM_E_INVALID_ARG;

	rc = RAND_bytes(buf, (int) len);
	if (rc != 1)
		return VTM_ERROR;

	return VTM_OK;
}

#else /* no crypto library supported */

int vtm_crypto_random_bytes(void *buf, size_t len)
{
	return VTM_E_NOT_SUPPORTED;
}

#endif 
