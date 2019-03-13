/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/core/error.h>
#include <vtm/sql/mysql/mysql.h>

int main(void)
{
	int rc;
	struct vtm_sql_module sql;
	struct vtm_sql_result result;
	vtm_sql_con *con;
	vtm_dataset *conf;
	size_t i;

	/* initialize mysql connector */
	rc = vtm_module_mysql_init(&sql);
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* database parameter */
	conf = vtm_dataset_new();
	if (!conf) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	vtm_dataset_set_string(conf, VTM_SQL_PAR_HOST, "127.0.0.1");
	vtm_dataset_set_int(conf, VTM_SQL_PAR_PORT, 3306);
	vtm_dataset_set_string(conf, VTM_SQL_PAR_DATABASE, "testdb");
	vtm_dataset_set_string(conf, VTM_SQL_PAR_USER, "testuser");
	vtm_dataset_set_string(conf, VTM_SQL_PAR_PASSWORD, "testpw");

	/* open connection */
	con = sql.con_new(conf);
	if (!con)
		goto end;

	/* query database */
	rc = vtm_sql_query(con, "SELECT * FROM users", &result);
	if (rc != VTM_OK)
		goto end;

	/* retrieve complete result set */
	rc = vtm_sql_result_fetch_all(&result);
	if (rc != VTM_OK) {
		vtm_sql_result_release(&result);
		goto end;
	}

	/* print results */
	for (i=0; i < result.row_count; i++) {
		printf("Row %lu: Name=%s\n", (unsigned long) i+1,
			vtm_dataset_get_string(&result.rows[i], "name"));
	}
	vtm_sql_result_release(&result);

end:
	/* free resources */
	vtm_sql_con_free(con);
	vtm_dataset_free(conf);

	/* shutdown sql module */
	vtm_module_sql_thread_end(&sql);
	vtm_module_sql_end(&sql);

	return 0;
}
