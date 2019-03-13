/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <stdlib.h>
#include <vtm/core/error.h>
#include <vtm/core/list.h>

#define VTM_LIST_COUNT      10

static void test_list(void)
{
	int rc, i, val;
	vtm_list *list;

	list = vtm_list_new(VTM_ELEM_INT, 8);
	VTM_TEST_ASSERT(list != NULL, "list allocation");

	/* add per varargs */
	for (i=0; i < VTM_LIST_COUNT; i++) {
		rc = vtm_list_add_va(list, i);
		VTM_TEST_CHECK(rc == VTM_OK, "list add va");
	}
	VTM_TEST_CHECK(vtm_list_size(list) == VTM_LIST_COUNT, "list size check");

	/* add typed */
	for (i=0; i < VTM_LIST_COUNT; i++) {
		rc = vtm_list_add(list, VTM_V_INT(i));
		VTM_TEST_CHECK(rc == VTM_OK, "list add");
	}
	VTM_TEST_CHECK(vtm_list_size(list) == VTM_LIST_COUNT * 2, "list size check");

	/* remove */
	for (i=0; i < VTM_LIST_COUNT; i++) {
		rc = vtm_list_remove(list, 0);
	}
	VTM_TEST_CHECK(vtm_list_size(list) == VTM_LIST_COUNT, "list size check");

	/* check values */
	for (i=0; i < VTM_LIST_COUNT; i++) {
		val = vtm_list_get_int(list, i);
		VTM_TEST_CHECK(val == i, "list value check");
	}

	vtm_list_free(list);
	VTM_TEST_PASSED("list free");
}

static void test_free_func(void)
{
	int rc, i;
	vtm_list *list;
	double *val;

	list = vtm_list_new(VTM_ELEM_POINTER, 8);
	VTM_TEST_ASSERT(list != NULL, "list allocation");

	vtm_list_set_free_func(list, free);

	for (i=0; i < VTM_LIST_COUNT; i++) {
		val = malloc(sizeof(double));
		VTM_TEST_ASSERT(val != NULL, "list value allocated");
		*val = i;
		vtm_list_add_va(list, val);
	}

	rc = vtm_list_remove(list, 0);
	VTM_TEST_CHECK(rc == VTM_OK, "list removal");

	vtm_list_free(list);
	VTM_TEST_PASSED("list free");
}

static void test_errors(void)
{
	int rc;
	vtm_list *list;

	list = vtm_list_new(VTM_ELEM_INT64, 10);
	VTM_TEST_ASSERT(list != NULL, "list allocation");

	/* wrong remove index */
	rc = vtm_list_remove(list, 4);
	VTM_TEST_CHECK(rc != VTM_OK, "list remove out of index");

	/* adding wrong type */
	rc = vtm_list_add(list, VTM_V_UINT64(5));
	VTM_TEST_CHECK(rc != VTM_OK, "list add wrong type");

	vtm_list_free(list);
	VTM_TEST_PASSED("list free");
}

static void test_type_ptr(void)
{
	int rc, a, *b;
	vtm_list *list;

	a = 5;

	list = vtm_list_new(VTM_ELEM_POINTER, 10);
	VTM_TEST_ASSERT(list != NULL,  "pointer list new");

	rc = vtm_list_add_va(list, &a);
	VTM_TEST_CHECK(rc == VTM_OK, "pointer list add_va");

	rc = vtm_list_add(list, VTM_V_PTR(&a));
	VTM_TEST_CHECK(rc == VTM_OK, "pointer list add");

	VTM_TEST_CHECK(vtm_list_size(list) == 2, "pointer list size");

	b = vtm_list_get_pointer(list, 0);
	VTM_TEST_CHECK(b == &a, "pointer addr check");
	VTM_TEST_CHECK(*b == a, "pointer value check");

	b = vtm_list_get_pointer(list, 1);
	VTM_TEST_CHECK(b == &a, "pointer addr check");
	VTM_TEST_CHECK(*b == a, "pointer value check");

	vtm_list_free(list);
	VTM_TEST_PASSED("pointer list free");
}

extern void test_vtm_core_list(void)
{
	VTM_TEST_LABEL("list");
	test_list();
	test_free_func();
	test_errors();
	test_type_ptr();
}
