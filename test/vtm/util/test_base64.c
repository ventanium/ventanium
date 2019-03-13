/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <string.h> /* strcmp() */
#include <vtm/core/error.h>
#include <vtm/util/base64.h>

#define TEST_INPUT       "Teststring for Encoding"
#define TEST_OUTPUT_STR  "VGVzdHN0cmluZyBmb3IgRW5jb2Rpbmc="

#define TEST_OUTPUT_BIN                                                     \
"AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1N" \
"jc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG" \
"1ub3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqO" \
"kpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna" \
"29zd3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w=="

#define TEST_BINARY_LEN  256

static void test_base64_encode_str(void)
{
	int rc;
	char result[VTM_BASE64_ENC_BUF_LEN(sizeof(TEST_INPUT)-1)];
	
	rc = vtm_base64_encode(TEST_INPUT, strlen(TEST_INPUT), result, sizeof(result));
	VTM_TEST_CHECK(rc == VTM_OK, "base64 encode string");
	
	rc = strcmp(result, TEST_OUTPUT_STR);
	VTM_TEST_CHECK(rc == 0, "base64 encode string check");
}

static void test_base64_decode_str(void)
{
	int rc;
	char result[VTM_BASE64_DEC_BUF_LEN(sizeof(TEST_OUTPUT_STR)-1)];
	size_t used;

	used = sizeof(result);
	rc = vtm_base64_decode(TEST_OUTPUT_STR, strlen(TEST_OUTPUT_STR), result, &used);
	VTM_TEST_CHECK(rc == VTM_OK, "base64 decode string");
	VTM_TEST_CHECK(used == strlen(TEST_INPUT), "base64 decode string length");

	result[used] = '\0';
	rc = strcmp(result, TEST_INPUT);
	VTM_TEST_CHECK(rc == 0, "base64 decode string check");
}

static void test_base64_encode_bin(void)
{
	int rc;
	unsigned char bin[TEST_BINARY_LEN];
	unsigned int i;
	char result[VTM_BASE64_ENC_BUF_LEN(sizeof(bin))];

	for (i=0; i < sizeof(bin); i++) {
		bin[i] = i;
	}

	rc = vtm_base64_encode(bin, sizeof(bin), result, sizeof(result));
	VTM_TEST_CHECK(rc == VTM_OK, "base64 encode binary");

	rc = strcmp(result, TEST_OUTPUT_BIN);
	VTM_TEST_CHECK(rc == 0, "base64 encode binary check");
}

static void test_base64_decode_bin(void)
{
	int rc;
	unsigned int i;
	unsigned char result[VTM_BASE64_DEC_BUF_LEN(sizeof(TEST_OUTPUT_BIN)-1)];
	size_t used;

	used = sizeof(result);
	rc = vtm_base64_decode(TEST_OUTPUT_BIN, strlen(TEST_OUTPUT_BIN), result, &used);
	VTM_TEST_CHECK(rc == VTM_OK, "base64 decode binary");
	VTM_TEST_CHECK(used == TEST_BINARY_LEN, "base64 decode binary length");
	
	for (i=0; i < used; i++) {
		if (result[i] != i) {
			VTM_TEST_FAILED("base64 decode binary check");
			return;
		}
	}
	VTM_TEST_PASSED("base64 decode binary check");
}

extern void test_vtm_util_base64(void)
{
	VTM_TEST_LABEL("base64");
	test_base64_encode_str();
	test_base64_decode_str();
	test_base64_encode_bin();
	test_base64_decode_bin();
}
