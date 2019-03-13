/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <vtm/core/math.h>
#include <vtm/core/dataset.h>

#define VTM_TEST_NUMBER(DS, FIELD, VAL)                                                                          \
	do {                                                                                                         \
	VTM_TEST_CHECK(vtm_dataset_get_int8(DS, FIELD) == (int8_t) VAL, "ds " FIELD " => int8");                     \
	VTM_TEST_CHECK(vtm_dataset_get_uint8(DS, FIELD) == (uint8_t) VAL, "ds " FIELD " => uint8");                  \
	VTM_TEST_CHECK(vtm_dataset_get_int16(DS, FIELD) == (int16_t) VAL, "ds " FIELD " => int16");                  \
	VTM_TEST_CHECK(vtm_dataset_get_uint16(DS, FIELD) == (uint16_t) VAL, "ds " FIELD " => uint16");               \
	VTM_TEST_CHECK(vtm_dataset_get_int32(DS, FIELD) == (int32_t) VAL, "ds " FIELD " => int32");                  \
	VTM_TEST_CHECK(vtm_dataset_get_uint32(DS, FIELD) == (uint32_t) VAL, "ds " FIELD " => uint32");               \
	VTM_TEST_CHECK(vtm_dataset_get_int64(DS, FIELD) == (int64_t) VAL, "ds " FIELD " => int64");                  \
	VTM_TEST_CHECK(vtm_dataset_get_uint64(DS, FIELD) == (uint64_t) VAL, "ds " FIELD " => uint64");               \
	VTM_TEST_CHECK(vtm_dataset_get_char(DS, FIELD) == (char) VAL, "ds " FIELD " => char");                       \
	VTM_TEST_CHECK(vtm_dataset_get_schar(DS, FIELD) == (signed char) VAL, "ds " FIELD " => signed char");        \
	VTM_TEST_CHECK(vtm_dataset_get_uchar(DS, FIELD) == (unsigned char) VAL, "ds " FIELD " => unsigned char");    \
	VTM_TEST_CHECK(vtm_dataset_get_short(DS, FIELD) == (short) VAL, "ds " FIELD " => signed short");             \
	VTM_TEST_CHECK(vtm_dataset_get_ushort(DS, FIELD) == (unsigned short) VAL, "ds " FIELD " => unsigned short"); \
	VTM_TEST_CHECK(vtm_dataset_get_int(DS, FIELD) == (int) VAL, "ds " FIELD " => signed int");                   \
	VTM_TEST_CHECK(vtm_dataset_get_uint(DS, FIELD) == (unsigned int) VAL, "ds " FIELD " => unsigned int");       \
	VTM_TEST_CHECK(vtm_dataset_get_long(DS, FIELD) == (long) VAL, "ds " FIELD " => signed long");                \
	VTM_TEST_CHECK(vtm_dataset_get_ulong(DS, FIELD) == (unsigned long) VAL, "ds " FIELD " => unsigned long");    \
	} while (0)

#define VTM_TEST_FLOATING_POINT(DS, FIELD, VAL)                                                                  \
	do {                                                                                                         \
	bool feq, deq;                                                                                               \
	feq = vtm_math_float_cmp(vtm_dataset_get_float(DS, FIELD), (float) VAL, 0.0001f);                            \
	VTM_TEST_CHECK(feq == true, "ds " FIELD " => float");                                                        \
	deq = vtm_math_double_cmp(vtm_dataset_get_double(DS, FIELD), (double) VAL, 0.0001);                          \
	VTM_TEST_CHECK(deq == true, "ds " FIELD " => double");                                                       \
	} while (0)

#define VTM_TEST_VARIANT(DS, FIELD, TYPE, ELEM, VAL)                                                             \
	do {                                                                                                         \
		struct vtm_variant *var;                                                                                 \
		var = vtm_dataset_get_variant(DS, FIELD);                                                                \
		VTM_TEST_ASSERT(var != NULL, "ds " FIELD " variant pointer");                                            \
		VTM_TEST_CHECK(var->type == TYPE, "ds " FIELD " variant type");                                          \
		VTM_TEST_CHECK(var->data.elem_ ## ELEM == VAL, "ds " FIELD " variant value");                            \
	} while (0)

#define VTM_TEST_DATASET(DS, FIELD, TYPE, ELEM, VAL)                                                             \
	do {                                                                                                         \
		vtm_dataset_set_ ## ELEM(ds, FIELD, VAL);                                                                \
		VTM_TEST_VARIANT(DS, FIELD, TYPE, ELEM, VAL);                                                            \
		VTM_TEST_NUMBER(DS, FIELD, VAL);                                                                         \
		VTM_TEST_FLOATING_POINT(DS, FIELD, VAL);                                                                 \
	} while (0)

#define VTM_TEST_DATASET_FLT(DS, FIELD, TYPE, ELEM, VAL)                                                         \
	do {                                                                                                         \
		vtm_dataset_set_ ## ELEM(ds, FIELD, VAL);                                                                \
		VTM_TEST_NUMBER(DS, FIELD, VAL);                                                                         \
		VTM_TEST_FLOATING_POINT(DS, FIELD, VAL);                                                                 \
	} while (0)

static void test_dataset(void)
{
	vtm_dataset *ds;

	ds = vtm_dataset_new();
	VTM_TEST_ASSERT(ds != NULL, "new dataset allocated");

	VTM_TEST_DATASET(ds, "int8", VTM_ELEM_INT8, int8, INT8_MIN);
	VTM_TEST_DATASET(ds, "uint8", VTM_ELEM_UINT8, uint8, UINT8_MAX);
	VTM_TEST_DATASET(ds, "int16", VTM_ELEM_INT16, int16, INT16_MIN);
	VTM_TEST_DATASET(ds, "uint16", VTM_ELEM_UINT16, uint16, UINT16_MAX);
	VTM_TEST_DATASET(ds, "int32", VTM_ELEM_INT32, int32, INT32_MIN);
	VTM_TEST_DATASET(ds, "uint32", VTM_ELEM_UINT32, uint32, UINT32_MAX);
	VTM_TEST_DATASET(ds, "int64", VTM_ELEM_INT64, int64, INT64_MIN);
	VTM_TEST_DATASET(ds, "uint64", VTM_ELEM_UINT64, uint64, UINT64_MAX);
	VTM_TEST_DATASET(ds, "char", VTM_ELEM_CHAR, char, CHAR_MAX);
	VTM_TEST_DATASET(ds, "schar", VTM_ELEM_SCHAR, schar, SCHAR_MIN);
	VTM_TEST_DATASET(ds, "uchar", VTM_ELEM_UCHAR, uchar, UCHAR_MAX);
	VTM_TEST_DATASET(ds, "short", VTM_ELEM_SHORT, short, SHRT_MIN);
	VTM_TEST_DATASET(ds, "ushort", VTM_ELEM_USHORT, ushort, USHRT_MAX);
	VTM_TEST_DATASET(ds, "int", VTM_ELEM_INT, int, INT_MIN);
	VTM_TEST_DATASET(ds, "uint", VTM_ELEM_UINT, uint, UINT_MAX);
	VTM_TEST_DATASET(ds, "long", VTM_ELEM_LONG, long, LONG_MIN);
	VTM_TEST_DATASET(ds, "ulong", VTM_ELEM_ULONG, ulong, LONG_MAX);
	VTM_TEST_DATASET_FLT(ds, "float", VTM_ELEM_FLOAT, float, 1.23456f);
	VTM_TEST_DATASET_FLT(ds, "double", VTM_ELEM_DOUBLE, double, 1.23456789);

	/* bool */
	vtm_dataset_set_bool(ds, "bool", true);
	VTM_TEST_VARIANT(ds, "bool", VTM_ELEM_BOOL, bool, true);
	VTM_TEST_NUMBER(ds, "bool", 1);
	VTM_TEST_FLOATING_POINT(ds, "bool", 1);

	/* unset field should be zero */
	VTM_TEST_NUMBER(ds, "unset", 0);

	vtm_dataset_free(ds);
	VTM_TEST_PASSED("allocation free");
}

extern void test_vtm_core_dataset(void)
{
	VTM_TEST_LABEL("dataset");
	test_dataset();
}
