/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "crypto.h"

#include <vtm/core/error.h>

#ifdef VTM_LIB_OPENSSL

#include <openssl/ssl.h>
#include <openssl/engine.h>
#include <openssl/conf.h>

#include <vtm/util/mutex.h>
#include <vtm/util/thread.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L

static vtm_mutex **locks;
static int num_locks;

#if OPENSSL_VERSION_NUMBER >= 0x10000000L

#define VTM_CRYPTO_SET_ID_CALLBACK   CRYPTO_THREADID_set_callback

static void vtm_module_crypto_cb_id(CRYPTO_THREADID *id)
{
	CRYPTO_THREADID_set_numeric(id, vtm_thread_get_current_id());
}

#else

#define VTM_CRYPTO_SET_ID_CALLBACK   CRYPTO_set_id_callback

static unsigned long vtm_module_crypto_cb_id(void)
{
	return vtm_thread_get_current_id();
}

#endif

static void vtm_module_crypto_cb_lock(int mode, int n, const char *file, int line)
{
	if (mode & CRYPTO_LOCK)
		vtm_mutex_lock(locks[n]);
	else
		vtm_mutex_unlock(locks[n]);
}

static int vtm_module_crypto_init_mt_callbacks(void)
{
	int rc, i;

	num_locks = CRYPTO_num_locks();

	locks = calloc(num_locks, sizeof(vtm_mutex*));
	if (!locks) {
		vtm_err_oom();
		return vtm_err_get_code();
	}

	for (i=0; i < num_locks; i++) {
		locks[i] = vtm_mutex_new();
		if (!locks[i]) {
			rc = vtm_err_get_code();
			goto err;
		}
	}

	VTM_CRYPTO_SET_ID_CALLBACK(vtm_module_crypto_cb_id);
	CRYPTO_set_locking_callback(vtm_module_crypto_cb_lock);

	return VTM_OK;

err:
	for (i=0; i < num_locks; i++) {
		if (locks[i])
			vtm_mutex_free(locks[i]);
	}

	return rc;
}

static void vtm_module_crypto_end_mt_callbacks(void)
{
	int i;

	for (i=0; i < num_locks; i++)
		vtm_mutex_free(locks[i]);

	free(locks);
}

#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */

int vtm_module_crypto_init(void)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	SSL_load_error_strings();
	SSL_library_init();
	return vtm_module_crypto_init_mt_callbacks();
#else
	return VTM_OK;
#endif
}

void vtm_module_crypto_end(void)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
#ifndef VTM_SYS_DARWIN
	FIPS_mode_set(0);
#endif
	ENGINE_cleanup();
	CONF_modules_unload(1);
	ERR_free_strings();
#if OPENSSL_VERSION_NUMBER > 0x10000000L
	ERR_remove_thread_state(0);
#else
	ERR_remove_state(0);
#endif
	EVP_cleanup();
	CRYPTO_set_locking_callback(NULL);
	CRYPTO_set_id_callback(NULL);
	CRYPTO_cleanup_all_ex_data();
	vtm_module_crypto_end_mt_callbacks();
#endif
}

#else

int vtm_module_crypto_init(void)
{
	return VTM_OK;
}

void vtm_module_crypto_end(void)
{
}

#endif
