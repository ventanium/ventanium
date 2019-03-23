/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <stdlib.h> /* free() */
#include <string.h> /* strcmp() */
#include <vtm/core/error.h>
#include <vtm/core/format.h>
#include <vtm/crypto/hash.h>

#define TEST_INPUT        "Teststring for hashing"
#define TEST_OUTPUT       "4b4e4d8dbff9926450fa2cb95b20e21cadd15c76"
#define TEST_HMAC_OUTPUT  "b22b88db48f3a3674de9f2a4738be16b3d9c0b5a4c000cd874" \
"3beafb08dba0c89642beeeea6d4c5a951f3fb27dff7e916d8139dd342b03dfabae3038cb5d0256"

static unsigned char HMAC_KEY[] = {
	0x13, 0x54, 0xa3, 0x75, 0x11, 0x82, 0xe9, 0x5f
};

static unsigned char HMAC_SALT[] = {
	0x82, 0x86, 0x46, 0xfa, 0x04, 0xca, 0x9e, 0x4d
};

static void init_modules(void)
{
	int rc;

	rc = vtm_module_crypto_init();
	VTM_TEST_ASSERT(rc == VTM_OK, "module crypto init");
}

static void end_modules(void)
{
	vtm_module_crypto_end();
}

static void test_hash_sha1(void)
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

static void test_hmac_sha512(void)
{
	int rc;
	unsigned char output[64];
	char output_hex[129];

	rc = vtm_crypto_hmac_sha512(HMAC_KEY, sizeof(HMAC_KEY),
		HMAC_SALT, sizeof(HMAC_SALT), output, sizeof(output), 1000);
	if (rc == VTM_E_NOT_SUPPORTED)
		return;
		
	VTM_TEST_CHECK(rc == VTM_OK, "sha512 HMAC execution");

	output_hex[vtm_fmt_hex_mem(output_hex, output, sizeof(output), false)] = '\0';
	rc = strcmp(TEST_HMAC_OUTPUT, output_hex);
	VTM_TEST_CHECK(rc == 0, "sha512 HMAC comparison");
}

extern void test_vtm_crypto_hash(void)
{
	VTM_TEST_LABEL("hash");
	init_modules();
	test_hash_sha1();
	test_hmac_sha512();
	end_modules();
}
