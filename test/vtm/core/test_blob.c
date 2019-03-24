/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <vtm/core/blob.h>

#define BLOB_SIZE     8

static void test_blob(void)
{
	int i;
	char *blob;

	blob = vtm_blob_new(BLOB_SIZE);
	VTM_TEST_ASSERT(blob != NULL, "blob alloc");

	VTM_TEST_CHECK(vtm_blob_size(blob) == BLOB_SIZE, "blob size check");

	for (i=0; i < BLOB_SIZE; i++) {
		blob[i] = i;
	}
	VTM_TEST_PASSED("blob write test");

	vtm_blob_free(blob);
	VTM_TEST_PASSED("blob free");
}

extern void test_vtm_core_blob(void)
{
	VTM_TEST_LABEL("blob");
	test_blob();
}
