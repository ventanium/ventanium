/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/core/error.h>
#include <vtm/sql/sqlite/sqlite.h>

int main(void)
{
	int rc;
	struct vtm_sql_module sql;
	struct vtm_sql_result result;
	vtm_sql_con *con;
	vtm_dataset *conf, *bind;
	size_t i;

	/* initialize sqlite connector */
	rc = vtm_module_sqlite_init(&sql);
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* database parameter */
	conf = vtm_dataset_new();
	if (!conf) {
		vtm_err_print();
		goto end_conf;
	}
	vtm_dataset_set_string(conf, VTM_SQL_PAR_DATABASE, "test_db");

	/* open connection */
	con = sql.con_new(conf);
	if (!con)
		goto end_conf;

	/* create table */
	rc = vtm_sql_execute(con, "CREATE TABLE IF NOT EXISTS users ("
			"user_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
			"cash INTEGER,"
			"name varchar(64) DEFAULT NULL)");
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end_con;
	}

	/* create values */
	bind = vtm_dataset_new();
	if (!bind) {
		vtm_err_print();
		goto end_con;
	}

	for (i=0; i < 10; i++) {
		vtm_dataset_set_int(bind, "money", i);
		vtm_dataset_set_int(bind, "name", i*2);
		rc = vtm_sql_execute_prepared(con, "INSERT INTO users(cash, name)"
			" VALUES(:money, :name)", bind);
		if (rc != VTM_OK) {
			vtm_err_print();
			goto end_bind;
		}
	}

	/* query result */
	rc = vtm_sql_query(con, "SELECT * FROM users WHERE cash > 5", &result);
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end_bind;
	}

	/* retrieve complete result set */
	rc = vtm_sql_result_fetch_all(&result);
	if (rc != VTM_OK) {
		vtm_sql_result_release(&result);
		goto end_bind;
	}

	/* print results */
	for (i=0; i < result.row_count; i++) {
		printf("Row %zu: user_id=%d cash=%d name=%s\n", i+1,
			vtm_dataset_get_int(&result.rows[i], "user_id"),
			vtm_dataset_get_int(&result.rows[i], "cash"),
			vtm_dataset_get_string(&result.rows[i], "name"));
	}
	vtm_sql_result_release(&result);

end_bind:
	vtm_dataset_free(bind);

end_con:
	vtm_sql_con_free(con);

end_conf:
	vtm_dataset_free(conf);

	/* shutdown sql module */
	vtm_module_sql_thread_end(&sql);
	vtm_module_sql_end(&sql);

	return 0;
}
