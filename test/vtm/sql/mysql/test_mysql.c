/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <vtm/fs/config.h>
#include <vtm/core/error.h>
#include <vtm/sql/mysql/mysql.h>

extern void test_sql_generic(vtm_sql_con *con);

struct vtm_sql_module sqlmod;

static void module_init(void)
{
	int rc;
	
	rc = vtm_module_mysql_init(&sqlmod);
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
	param = vtm_config_file_read("./test/data/sql/mysql/mysql.conf");
	VTM_TEST_ASSERT(param != NULL, "mysql.conf loaded");

	/* open connection */
	con = sqlmod.con_new(param);
	VTM_TEST_ASSERT(con != NULL, "mysql con opened");

	/* run generic sql tests */
	test_sql_generic(con);
	VTM_TEST_PASSED("mysql con test");

	/* free config */
	vtm_dataset_free(param);
}

extern void test_vtm_sql_mysql(void)
{
	VTM_TEST_LABEL("mysql");
	module_init();
	test_connection();
	module_end();
}
