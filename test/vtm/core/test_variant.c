/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <string.h>
#include <vtm/core/math.h>
#include <vtm/core/variant.h>

#define VTM_TEST_VALUE(VAR, TYPE, ELEM, VAL)                                                            \
	do {                                                                                                \
		VTM_TEST_CHECK(VAR.type == TYPE, "variant type check");                                         \
		VTM_TEST_CHECK(VAR.data.elem_ ## ELEM == VAL, "variant value check");                           \
	} while (0)

#define VTM_TEST_NUMERIC_CONVERSION(VAR, VAL)                                                           \
	do {                                                                                                \
	VTM_TEST_CHECK(vtm_variant_as_int8(VAR) == (int8_t) VAL, "variant value int8");                     \
	VTM_TEST_CHECK(vtm_variant_as_uint8(VAR) == (uint8_t) VAL, "variant value uint8");                  \
	VTM_TEST_CHECK(vtm_variant_as_int16(VAR) == (int16_t) VAL, "variant value int16");                  \
	VTM_TEST_CHECK(vtm_variant_as_uint16(VAR) == (uint16_t) VAL, "variant value uint16");               \
	VTM_TEST_CHECK(vtm_variant_as_int32(VAR) == (int32_t) VAL, "variant value int32");                  \
	VTM_TEST_CHECK(vtm_variant_as_uint32(VAR) == (uint32_t) VAL, "variant value uint32");               \
	VTM_TEST_CHECK(vtm_variant_as_int64(VAR) == (int64_t) VAL, "variant value int64");                  \
	VTM_TEST_CHECK(vtm_variant_as_uint64(VAR) == (uint64_t) VAL, "variant value uint64");               \
	VTM_TEST_CHECK(vtm_variant_as_char(VAR) == (char) VAL, "variant value char");                       \
	VTM_TEST_CHECK(vtm_variant_as_schar(VAR) == (signed char) VAL, "variant value signed char");        \
	VTM_TEST_CHECK(vtm_variant_as_uchar(VAR) == (unsigned char) VAL, "variant value unsigned char");    \
	VTM_TEST_CHECK(vtm_variant_as_short(VAR) == (short) VAL, "variant value signed short");             \
	VTM_TEST_CHECK(vtm_variant_as_ushort(VAR) == (unsigned short) VAL, "variant value unsigned short"); \
	VTM_TEST_CHECK(vtm_variant_as_int(VAR) == (int) VAL, "variant value signed int");                   \
	VTM_TEST_CHECK(vtm_variant_as_uint(VAR) == (unsigned int) VAL, "variant value unsigned int");       \
	VTM_TEST_CHECK(vtm_variant_as_long(VAR) == (long) VAL, "variant value signed long");                \
	VTM_TEST_CHECK(vtm_variant_as_ulong(VAR) == (unsigned long) VAL, "variant value unsigned long");    \
	} while (0)

#define VTM_TEST_FLOATING_POINT_CONVERSION(VAR, VAL)                                                    \
	do {                                                                                                \
	bool feq, deq;                                                                                      \
	feq = vtm_math_float_cmp(vtm_variant_as_float(VAR), (float) VAL, 0.0001f);                          \
	VTM_TEST_CHECK(feq == true, "variant value float");                                                 \
	deq = vtm_math_double_cmp(vtm_variant_as_double(VAR), (double) VAL, 0.0001f);                       \
	VTM_TEST_CHECK(deq == true, "variant value double");                                                \
	} while (0)

#define VTM_TEST_VARIANT(FUNC, TYPE, ELEM, VAL)                                                         \
	do {                                                                                                \
		struct vtm_variant var = FUNC(VAL);                                                             \
		VTM_TEST_VALUE(var, TYPE, ELEM, VAL);                                                           \
		VTM_TEST_NUMERIC_CONVERSION(&var, VAL);                                                         \
		VTM_TEST_FLOATING_POINT_CONVERSION(&var, VAL);                                                  \
	} while (0)

#define VTM_TEST_VARIANT_FLT(FUNC, TYPE, ELEM, VAL)                                                     \
	do {                                                                                                \
		struct vtm_variant var = FUNC(VAL);                                                             \
		VTM_TEST_CHECK(var.type == TYPE, "floating variant type check");                                \
		VTM_TEST_NUMERIC_CONVERSION(&var, VAL);                                                         \
		VTM_TEST_FLOATING_POINT_CONVERSION(&var, VAL);                                                  \
	} while (0)

static void test_variant(void)
{
	VTM_TEST_VARIANT(VTM_V_INT8, VTM_ELEM_INT8, int8, INT8_MIN);
	VTM_TEST_VARIANT(VTM_V_UINT8, VTM_ELEM_UINT8, uint8, UINT8_MAX);
	VTM_TEST_VARIANT(VTM_V_INT16, VTM_ELEM_INT16, int16, INT16_MIN);
	VTM_TEST_VARIANT(VTM_V_UINT16, VTM_ELEM_UINT16, uint16, UINT16_MAX);
	VTM_TEST_VARIANT(VTM_V_INT32, VTM_ELEM_INT32, int32, INT32_MIN);
	VTM_TEST_VARIANT(VTM_V_UINT32, VTM_ELEM_UINT32, uint32, UINT32_MAX);
	VTM_TEST_VARIANT(VTM_V_INT64, VTM_ELEM_INT64, int64, INT64_MIN);
	VTM_TEST_VARIANT(VTM_V_UINT64, VTM_ELEM_UINT64, uint64, UINT64_MAX);
	VTM_TEST_VARIANT(VTM_V_CHAR, VTM_ELEM_CHAR, char, CHAR_MAX);
	VTM_TEST_VARIANT(VTM_V_SCHAR, VTM_ELEM_SCHAR, schar, SCHAR_MAX);
	VTM_TEST_VARIANT(VTM_V_UCHAR, VTM_ELEM_UCHAR, uchar, UCHAR_MAX);
	VTM_TEST_VARIANT(VTM_V_SHORT, VTM_ELEM_SHORT, short, SHRT_MIN);
	VTM_TEST_VARIANT(VTM_V_USHORT, VTM_ELEM_USHORT, ushort, USHRT_MAX);
	VTM_TEST_VARIANT(VTM_V_INT, VTM_ELEM_INT, int, INT_MIN);
	VTM_TEST_VARIANT(VTM_V_UINT, VTM_ELEM_UINT, uint, UINT_MAX);
	VTM_TEST_VARIANT(VTM_V_LONG, VTM_ELEM_LONG, long, LONG_MIN);
	VTM_TEST_VARIANT(VTM_V_ULONG, VTM_ELEM_ULONG, ulong, ULONG_MAX);
	VTM_TEST_VARIANT_FLT(VTM_V_FLOAT, VTM_ELEM_FLOAT, float, 1.23456f);
	VTM_TEST_VARIANT_FLT(VTM_V_DOUBLE, VTM_ELEM_DOUBLE, double, 1.23456789);
}

static void test_string_conversion(void)
{
	bool feq, deq;
	char val[] = "12874.34";
	int intval = 12874;
	float floatval = 12874.34f;
	double doubleval = 12874.34;

	struct vtm_variant var = VTM_V_STR(val);
	VTM_TEST_CHECK(var.type == VTM_ELEM_STRING, "string allocated");

	VTM_TEST_CHECK(vtm_variant_as_bool(&var) == false, "string => bool");
	VTM_TEST_CHECK(vtm_variant_as_int64(&var) == intval, "string => int64");
	VTM_TEST_CHECK(strcmp(vtm_variant_as_str(&var), val) == 0, "string => string");

	feq = vtm_math_float_cmp(vtm_variant_as_float(&var), floatval, 0.001f);
	VTM_TEST_CHECK(feq == true, "string => float");

	deq = vtm_math_double_cmp(vtm_variant_as_double(&var), doubleval, 0.001);
	VTM_TEST_CHECK(deq == true, "string => double");
}

extern void test_vtm_core_variant(void)
{
	VTM_TEST_LABEL("variant");
	test_variant();
	test_string_conversion();
}
