/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <stdio.h>
#include <vtm/core/error.h>
#include <vtm/fs/config.h>
#include <vtm/sql/sqlite/sqlite.h>

extern void test_sql_generic(vtm_sql_con *con);

struct vtm_sql_module sqlmod;

static void module_init(void)
{
	int rc;
	
	rc = vtm_module_sqlite_init(&sqlmod);
	VTM_TEST_ASSERT(rc == VTM_OK, "mysql module init");
}

static void module_end(void)
{
	vtm_module_sql_thread_end(&sqlmod);
	vtm_module_sql_end(&sqlmod);
}

static void test_connection(void)
{
	vtm_sql_con *con;
	vtm_dataset *param;

	/* read config */
	param = vtm_config_file_read("./test/data/sql/sqlite/sqlite.conf");
	VTM_TEST_ASSERT(param != NULL, "sqlite.conf loaded");

	/* open connection */
	con = sqlmod.con_new(param);
	VTM_TEST_ASSERT(con != NULL, "sqlite con opened");

	/* run generic sql tests */
	test_sql_generic(con);
	VTM_TEST_PASSED("sqlite con test");

	/* delete test db file */
	remove(vtm_dataset_get_string(param, "DATABASE"));

	/* free config */
	vtm_dataset_free(param);
}

extern void test_vtm_sql_sqlite(void)
{
	VTM_TEST_LABEL("sqlite");
	module_init();
	test_connection();
	module_end();
}
