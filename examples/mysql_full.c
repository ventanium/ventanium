/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/core/error.h>
#include <vtm/sql/mysql/mysql.h>

void print_row(vtm_dataset *row, char **columns, size_t col_count)
{
	size_t i;

	for (i=0; i < col_count; i++) {
		if (i > 0)
			printf(" ");

		printf("%s=%s", columns[i], vtm_dataset_get_string(row, columns[i]));
	}
	printf("\n");
}

int main(void)
{
	int rc;
	struct vtm_sql_module sql;
	struct vtm_sql_result result;
	struct vtm_sql_stmt stmt;
	vtm_sql_con *con;
	vtm_dataset *conf, *row, *bind;
	size_t i;

	con = NULL;
	bind = NULL;

	/* initialize mysql connector */
	rc = vtm_module_mysql_init(&sql);
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* database parameter */
	conf = vtm_dataset_new();
	if (!conf) {
		rc = vtm_err_get_code();
		goto end;
	}

	vtm_dataset_set_string(conf, VTM_SQL_PAR_HOST, "127.0.0.1");
	vtm_dataset_set_int(conf, VTM_SQL_PAR_PORT, 3306);
	vtm_dataset_set_string(conf, VTM_SQL_PAR_DATABASE, "testdb");
	vtm_dataset_set_string(conf, VTM_SQL_PAR_USER, "testuser");
	vtm_dataset_set_string(conf, VTM_SQL_PAR_PASSWORD, "testpw");

	/* open connection */
	con = sql.con_new(conf);
	if (!con) {
		rc = vtm_err_get_code();
		goto end;
	}

	/* create new table */
	rc = vtm_sql_execute(con, "DROP TABLE IF EXISTS users");
	if (rc != VTM_OK)
		goto end;

	rc = vtm_sql_execute(con, "CREATE TABLE IF NOT EXISTS users ("
				"user_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,"
				"name varchar(64) DEFAULT NULL,"
				"cash int(11) NOT NULL,"
				"ts_update timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,"
				"PRIMARY KEY(user_id)"
				") DEFAULT CHARSET=utf8");
	if (rc != VTM_OK)
		goto end;

	/* create dataset for binding parameters */
	bind = vtm_dataset_newh(VTM_DS_HINT_STATIC_NAMES);
	if (!bind) {
		rc = vtm_err_get_code();
		goto end;
	}

	/* create prepared statement */
	rc = vtm_sql_prepare(con, "INSERT INTO users(name, cash) VALUES(:n,:c)",
		                 &stmt);
	if (rc != VTM_OK)
		goto end;

	/* create some rows */
	vtm_sql_set_auto_commit(con, false);
	
	for (i=0; i < 10000; i++) {
		vtm_dataset_set_int(bind, "n", i);
		vtm_dataset_set_int(bind, "c", i*2);
		
		rc = vtm_sql_stmt_bind(&stmt, bind);
		if (rc != VTM_OK) {
			vtm_sql_stmt_release(&stmt);
			goto end;
		}

		rc = vtm_sql_stmt_execute(&stmt);
		if (rc != VTM_OK) {
			vtm_sql_stmt_release(&stmt);
			goto end;
		}
	}

	vtm_sql_stmt_release(&stmt);
	vtm_sql_set_auto_commit(con, true);

	/* query database */
	vtm_dataset_clear(bind);
	vtm_dataset_set_int(bind, "MIN", 5000);
	vtm_dataset_set_int(bind, "MAX", 5020);
	rc = vtm_sql_query_prepared(con,
		"SELECT * FROM users WHERE cash > :MIN && cash < :MAX",
		bind, &result);
	if (rc != VTM_OK)
		goto end;

	/* print results */
	row = vtm_dataset_new();
	if (!row) {
		rc = vtm_err_get_code();
		goto end;
	}
	
	i = 0;
	while ((rc = vtm_sql_result_fetch_row(&result, row)) == VTM_OK) {
		printf("Row %lu: ", (unsigned long) ++i);
		print_row(row, result.columns, result.col_count);
	}
	vtm_sql_result_release(&result);
	
	if (rc == VTM_E_IO_EOF)
		rc = VTM_OK;

end:
	if (rc != VTM_OK)
		vtm_err_print();

	/* free resources */
	vtm_sql_con_free(con);
	vtm_dataset_free(row);
	vtm_dataset_free(bind);
	vtm_dataset_free(conf);

	/* shutdown sql module */
	vtm_module_sql_thread_end(&sql);
	vtm_module_sql_end(&sql);

	return 0;
}
