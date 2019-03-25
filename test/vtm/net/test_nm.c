/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <string.h>
#include <vtm/core/blob.h>
#include <vtm/core/dataset.h>
#include <vtm/core/math.h>

void test_nm_prepare_request(vtm_dataset *msg)
{
	void *blob;

	blob = vtm_blob_new(1);
	VTM_TEST_ASSERT(blob != NULL, "nm blob allocated");
	memset(blob, 'T', 1);

	vtm_dataset_set_int8(msg, "INT8", -7);
	vtm_dataset_set_uint8(msg, "UINT8", 7);
	vtm_dataset_set_int16(msg, "INT16", -15);
	vtm_dataset_set_uint16(msg, "UINT16", 15);
	vtm_dataset_set_int32(msg, "INT32", -31);
	vtm_dataset_set_uint32(msg, "UINT32", 31);
	vtm_dataset_set_int64(msg, "INT64", -63);
	vtm_dataset_set_uint64(msg, "UINT64", 63);

	vtm_dataset_set_bool(msg, "BOOL", true);
	vtm_dataset_set_char(msg, "CHAR", 63);
	vtm_dataset_set_schar(msg, "SCHAR", -126);
	vtm_dataset_set_uchar(msg, "UCHAR", 254);
	vtm_dataset_set_float(msg, "FLOAT", 1.23456f);
	vtm_dataset_set_double(msg, "DOUBLE", 1.2345678);
	vtm_dataset_set_string(msg, "STR", "Hello");
	vtm_dataset_set_blob(msg, "BLOB", blob);
}

void test_nm_prepare_response(vtm_dataset *msg)
{
	int8_t i8;
	uint8_t u8;
	int16_t i16;
	uint16_t u16;
	int32_t i32;
	uint32_t u32;
	int64_t i64;
	uint64_t u64;

	bool bval;
	char c;
	signed char sc;
	unsigned char uc;
	float fval;
	double dval;
	const char *str;
	const void *blob;
	void *blob_res;

	i8  = vtm_dataset_get_int8(msg, "INT8");
	u8  = vtm_dataset_get_uint8(msg, "UINT8");
	i16 = vtm_dataset_get_int16(msg, "INT16");
	u16 = vtm_dataset_get_uint16(msg, "UINT16");
	i32 = vtm_dataset_get_int32(msg, "INT32");
	u32 = vtm_dataset_get_uint32(msg, "UINT32");
	i64 = vtm_dataset_get_int64(msg, "INT64");
	u64 = vtm_dataset_get_uint64(msg, "UINT64");

	bval = vtm_dataset_get_bool(msg, "BOOL");
	c = vtm_dataset_get_char(msg, "CHAR");
	sc = vtm_dataset_get_schar(msg, "SCHAR");
	uc = vtm_dataset_get_uchar(msg, "UCHAR");
	fval = vtm_dataset_get_float(msg, "FLOAT");
	dval = vtm_dataset_get_double(msg, "DOUBLE");
	str = vtm_dataset_get_string(msg, "STR");
	blob = vtm_dataset_get_blob(msg, "BLOB");

	if (i8 == -7)
		vtm_dataset_set_int8(msg, "INT8", i8 - 1);
	if (u8 == 7)
		vtm_dataset_set_uint8(msg, "UINT8", u8 + 1);
	if (i16 == -15)
		vtm_dataset_set_int16(msg, "INT16", i16 - 1);
	if (u16 == 15)
		vtm_dataset_set_uint16(msg, "UINT16", u16 + 1);
	if (i32 == -31)
		vtm_dataset_set_int32(msg, "INT32", i32 - 1);
	if (u32 == 31)
		vtm_dataset_set_uint32(msg, "UINT32", u32 + 1);
	if (i64 == -63)
		vtm_dataset_set_int64(msg, "INT64", i64 - 1);
	if (u64 == 63)
		vtm_dataset_set_uint64(msg, "UINT64", u64 + 1);

	if (bval)
		vtm_dataset_set_bool(msg, "BOOL", false);
	if (c == 63)
		vtm_dataset_set_char(msg, "CHAR", c + 1);
	if (sc == -126)
		vtm_dataset_set_schar(msg, "SCHAR", sc - 1);
	if (uc == 254)
		vtm_dataset_set_uchar(msg, "UCHAR", uc + 1);
	if (vtm_math_float_cmp(fval,  1.23456f, 0.0001f))
		vtm_dataset_set_float(msg, "FLOAT", 1.0f);
	if (vtm_math_double_cmp(dval, 1.2345678, 0.0001f))
		vtm_dataset_set_double(msg, "DOUBLE", 1.0);
	if (strcmp(str, "Hello") == 0)
		vtm_dataset_set_string(msg, "STR", "olleH");

	if (blob && ((char*) blob)[0] == 'T') {
		blob_res = vtm_blob_new(1);
		if (blob_res) {
			memset(blob_res, 'Z', 1);
			vtm_dataset_set_blob(msg, "BLOB", blob_res);
		}
	}
}

void test_nm_check_response(vtm_dataset *msg)
{
	const char *str;
	const void *blob;

	VTM_TEST_CHECK(vtm_dataset_get_int8(msg, "INT8") == -8, "nm response int8");
	VTM_TEST_CHECK(vtm_dataset_get_uint8(msg, "UINT8") == 8, "nm response uint8");
	VTM_TEST_CHECK(vtm_dataset_get_int16(msg, "INT16") == -16, "nm response int16");
	VTM_TEST_CHECK(vtm_dataset_get_uint16(msg, "UINT16") == 16, "nm response uint16");
	VTM_TEST_CHECK(vtm_dataset_get_int32(msg, "INT32") == -32, "nm response int32");
	VTM_TEST_CHECK(vtm_dataset_get_uint32(msg, "UINT32") == 32, "nm response uint32");
	VTM_TEST_CHECK(vtm_dataset_get_int64(msg, "INT64") == -64, "nm response int64");
	VTM_TEST_CHECK(vtm_dataset_get_uint64(msg, "UINT64") == 64, "nm response uint64");

	VTM_TEST_CHECK(vtm_dataset_get_bool(msg, "BOOL") == false, "nm response bool");
	VTM_TEST_CHECK(vtm_dataset_get_schar(msg, "CHAR") == 64, "nm response char");
	VTM_TEST_CHECK(vtm_dataset_get_schar(msg, "SCHAR") == -127, "nm response schar");
	VTM_TEST_CHECK(vtm_dataset_get_uchar(msg, "UCHAR") == 255, "nm response uchar");
	VTM_TEST_CHECK(vtm_dataset_get_float(msg, "FLOAT") == 1.0f, "nm response float");
	VTM_TEST_CHECK(vtm_dataset_get_double(msg, "DOUBLE") == 1.0, "nm response double");

	str = vtm_dataset_get_string(msg, "STR");
	VTM_TEST_ASSERT(str != NULL, "nm response str check");
	VTM_TEST_CHECK(strcmp(str, "olleH") == 0, "nm response str");

	blob = vtm_dataset_get_blob(msg, "BLOB");
	VTM_TEST_ASSERT(blob != NULL, "nm response blob check");
	VTM_TEST_CHECK(((char*) blob)[0] == 'Z', "nm response blob");
}
