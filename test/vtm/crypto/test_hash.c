/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <stdlib.h> /* free() */
#include <string.h> /* strcmp() */
#include <vtm/core/error.h>
#include <vtm/core/format.h>
#include <vtm/crypto/hash.h>

#define TEST_INPUT	"Teststring for hashing"
#define TEST_OUTPUT	"4b4e4d8dbff9926450fa2cb95b20e21cadd15c76"

static void test_hash(void)
{
	int rc, n;
	unsigned char hash[VTM_CRYPTO_SHA1_LEN];
	char *hash_str;
	
	rc = vtm_crypto_sha1(TEST_INPUT, strlen(TEST_INPUT), hash, sizeof(hash));
	VTM_TEST_CHECK(rc == VTM_OK, "sha1 hash");
	
	hash_str = malloc(VTM_CRYPTO_SHA1_LEN * 2 + 1);
	n = vtm_fmt_hex_mem(hash_str, hash, VTM_CRYPTO_SHA1_LEN, false);
	hash_str[n] = '\0';
		
	n = strcmp(hash_str, TEST_OUTPUT);
	VTM_TEST_CHECK(n == 0, "sha1 hash check");

	free(hash_str);
}

extern void test_vtm_crypto_hash(void)
{
	VTM_TEST_LABEL("hash");
	test_hash();
}
