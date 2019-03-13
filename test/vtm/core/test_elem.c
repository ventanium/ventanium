/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <vtm/core/elem.h>
#include <vtm/core/math.h>

#define VTM_TEST_NUMERIC_CONVERSION(EL, TYPE, VAL)                                                     \
	do {                                                                                               \
	VTM_TEST_CHECK(vtm_elem_as_int8(TYPE, EL) == (int8_t) VAL, "elem value int8");                     \
	VTM_TEST_CHECK(vtm_elem_as_uint8(TYPE, EL) == (uint8_t) VAL, "elem value uint8");                  \
	VTM_TEST_CHECK(vtm_elem_as_int16(TYPE, EL) == (int16_t) VAL, "elem value int16");                  \
	VTM_TEST_CHECK(vtm_elem_as_uint16(TYPE, EL) == (uint16_t) VAL, "elem value uint16");               \
	VTM_TEST_CHECK(vtm_elem_as_int32(TYPE, EL) == (int32_t) VAL, "elem value int32");                  \
	VTM_TEST_CHECK(vtm_elem_as_uint32(TYPE, EL) == (uint32_t) VAL, "elem value uint32");               \
	VTM_TEST_CHECK(vtm_elem_as_int64(TYPE, EL) == (int64_t) VAL, "elem value int64");                  \
	VTM_TEST_CHECK(vtm_elem_as_uint64(TYPE, EL) == (uint64_t) VAL, "elem value uint64");               \
	VTM_TEST_CHECK(vtm_elem_as_char(TYPE, EL) == (char) VAL, "elem value char");                       \
	VTM_TEST_CHECK(vtm_elem_as_schar(TYPE, EL) == (signed char) VAL, "elem value signed char");        \
	VTM_TEST_CHECK(vtm_elem_as_uchar(TYPE, EL) == (unsigned char) VAL, "elem value unsigned char");    \
	VTM_TEST_CHECK(vtm_elem_as_short(TYPE, EL) == (short) VAL, "elem value signed short");             \
	VTM_TEST_CHECK(vtm_elem_as_ushort(TYPE, EL) == (unsigned short) VAL, "elem value unsigned short"); \
	VTM_TEST_CHECK(vtm_elem_as_int(TYPE, EL) == (int) VAL, "elem value signed int");                   \
	VTM_TEST_CHECK(vtm_elem_as_uint(TYPE, EL) == (unsigned int) VAL, "elem value unsigned int");       \
	VTM_TEST_CHECK(vtm_elem_as_long(TYPE, EL) == (long) VAL, "elem value signed long");                \
	VTM_TEST_CHECK(vtm_elem_as_ulong(TYPE, EL) == (unsigned long) VAL, "elem value unsigned long");    \
	} while (0)

#define VTM_TEST_FLOATING_POINT_CONVERSION(EL, TYPE, VAL)                                              \
	do {                                                                                               \
	bool feq, deq;                                                                                     \
	feq = vtm_math_float_cmp(vtm_elem_as_float(TYPE, EL), (float) VAL, 0.0001f);                       \
	VTM_TEST_CHECK(feq == true, "elem value float");                                                   \
	deq = vtm_math_double_cmp(vtm_elem_as_double(TYPE, EL), (double) VAL, 0.0001);                     \
	VTM_TEST_CHECK(deq == true, "elem value double");                                                  \
	} while (0)

#define VTM_TEST_ELEM(TYPE, ELEM, VAL)                                                                 \
	do {                                                                                               \
		union vtm_elem el;                                                                             \
		el.elem_ ## ELEM = VAL;                                                                        \
		VTM_TEST_NUMERIC_CONVERSION(&el, TYPE, VAL);                                                   \
		VTM_TEST_FLOATING_POINT_CONVERSION(&el, TYPE, VAL);                                            \
	} while (0)

static void test_elem(void)
{
	VTM_TEST_ELEM(VTM_ELEM_INT8, int8, INT8_MIN);
	VTM_TEST_ELEM(VTM_ELEM_UINT8, uint8, UINT8_MAX);
	VTM_TEST_ELEM(VTM_ELEM_INT16, int16, INT16_MIN);
	VTM_TEST_ELEM(VTM_ELEM_UINT16, uint16, UINT16_MAX);
	VTM_TEST_ELEM(VTM_ELEM_INT32, int32, INT32_MIN);
	VTM_TEST_ELEM(VTM_ELEM_UINT32, uint32, UINT32_MAX);
	VTM_TEST_ELEM(VTM_ELEM_INT64, int64, INT64_MIN);
	VTM_TEST_ELEM(VTM_ELEM_UINT64, uint64, UINT64_MAX);
	VTM_TEST_ELEM(VTM_ELEM_CHAR, char, CHAR_MAX);
	VTM_TEST_ELEM(VTM_ELEM_SCHAR, schar, SCHAR_MAX);
	VTM_TEST_ELEM(VTM_ELEM_UCHAR, uchar, UCHAR_MAX);
	VTM_TEST_ELEM(VTM_ELEM_SHORT, short, SHRT_MIN);
	VTM_TEST_ELEM(VTM_ELEM_USHORT, ushort, USHRT_MAX);
	VTM_TEST_ELEM(VTM_ELEM_INT, int, INT_MIN);
	VTM_TEST_ELEM(VTM_ELEM_UINT, uint, UINT_MAX);
	VTM_TEST_ELEM(VTM_ELEM_LONG, long, LONG_MIN);
	VTM_TEST_ELEM(VTM_ELEM_ULONG, ulong, ULONG_MAX);
	VTM_TEST_ELEM(VTM_ELEM_FLOAT, float, 1.23456f);
	VTM_TEST_ELEM(VTM_ELEM_DOUBLE, double, 1.23456789);
}

extern void test_vtm_core_elem(void)
{
	VTM_TEST_LABEL("elem");
	test_elem();
}
