/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <float.h>
#include <vtm/core/error.h>
#include <vtm/util/serialization.h>

static void test_float_packing(float val)
{
	int rc;
	uint32_t var32;
	float flt_val;

	rc = vtm_pack_float(val, &var32);
	VTM_TEST_CHECK(rc == VTM_OK, "pack float");

	rc = vtm_unpack_float(var32, &flt_val);
	VTM_TEST_CHECK(rc == VTM_OK, "unpack float");
	VTM_TEST_CHECK(flt_val == val, "check float result");
}

static void test_double_packing(double val)
{
	int rc;
	uint64_t var64;
	double dbl_val;

	rc = vtm_pack_double(val, &var64);
	VTM_TEST_CHECK(rc == VTM_OK, "pack double");

	rc = vtm_unpack_double(var64, &dbl_val);
	VTM_TEST_CHECK(rc == VTM_OK, "unpack double");
	VTM_TEST_CHECK(dbl_val == val, "check double result");
}

static void test_packing(void)
{
	VTM_TEST_LABEL("pack-flt-min");
	test_float_packing(FLT_MIN);
	VTM_TEST_LABEL("pack-flt-max");
	test_float_packing(FLT_MAX);
	VTM_TEST_LABEL("pack-flt-zero");
	test_float_packing(0.0f);

	VTM_TEST_LABEL("pack-dbl-min");
	test_double_packing(DBL_MIN);
	VTM_TEST_LABEL("pack-dbl-max");
	test_double_packing(DBL_MAX);
	VTM_TEST_LABEL("pack-dbl-zero");
	test_double_packing(0.0);
}

extern void test_vtm_util_serialization(void)
{
	VTM_TEST_LABEL("serialization");
	test_packing();
}
