/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include <vtf.h>

#include <string.h>
#include <vtm/fs/config.h>

static void test_config(void)
{
	vtm_dataset *cfg = vtm_config_file_read("./test/data/fs/test.conf");
	VTM_TEST_ASSERT(cfg != NULL, "config file read");

	vtm_list *entries = vtm_dataset_entryset(cfg);
	VTM_TEST_CHECK(vtm_list_size(entries) == 3, "entry number check");
	vtm_list_free(entries);

	VTM_TEST_CHECK(vtm_dataset_get_bool(cfg, "TEST_BOOL") == true, "cfg bool");
	VTM_TEST_CHECK(vtm_dataset_get_long(cfg, "TEST_NUMBER") == 12345, "cfg number");
	VTM_TEST_CHECK(strcmp(vtm_dataset_get_string(cfg, "TEST_STRING"), "/test/path/") == 0, "cfg string");

	vtm_dataset_free(cfg);
	VTM_TEST_PASSED("config free");
}

extern void test_vtm_fs_config(void)
{
	VTM_TEST_LABEL("config");
	test_config();
}
