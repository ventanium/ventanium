/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <string.h> /* strcmp() */
#include <vtm/core/error.h>
#include <vtm/sql/sql.h>

#define SQL_INSERT   "INSERT INTO users(number, msg) VALUES(10, NULL)"
#define SQL_INS_PREP "INSERT INTO users(number, msg) VALUES(:num, :msg)"

static void test_sql_simple(vtm_sql_con *con, vtm_dataset *bind)
{
	int rc;
	struct vtm_variant vnull;
	struct vtm_sql_result result;
	unsigned int row_count;
	vtm_dataset *row;
	
	/* Execute - DROP */
	rc = vtm_sql_execute(con, "DROP TABLE IF EXISTS users");
	VTM_TEST_ASSERT(rc == VTM_OK, "sql drop");
	
	/* Execute - CREATE */
	rc = vtm_sql_execute(con, "CREATE TABLE IF NOT EXISTS users ("
				"user_id INTEGER NOT NULL /*!40101 AUTO_INCREMENT */,"
				"number int(11) NOT NULL,"
				"msg varchar(64) DEFAULT NULL,"
				"ts_update timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,"
				"PRIMARY KEY(user_id)"
				")");
	VTM_TEST_ASSERT(rc == VTM_OK, "sql create");
	
	/* Execute - INSERT */
	rc = vtm_sql_execute(con, SQL_INSERT);
	VTM_TEST_CHECK(rc == VTM_OK, "sql insert");
	
	/* ExecutePrepared - INSERT */
	vtm_dataset_set_int(bind, "num", 10);
	vtm_dataset_set_string(bind, "msg", "ABC");
	rc = vtm_sql_execute_prepared(con, SQL_INS_PREP, bind);
	VTM_TEST_CHECK(rc == VTM_OK, "sql insert prepared");
	
	/* EcxecutePrepared - INSERT with NULL */
	vtm_dataset_clear(bind);
	vnull = VTM_V_NULL();
	vtm_dataset_set_int(bind, "num", 20);
	vtm_dataset_set_variant(bind, "msg", &vnull);
	rc = vtm_sql_execute_prepared(con, SQL_INS_PREP, bind);
	VTM_TEST_CHECK(rc == VTM_OK, "sql insert prepared with NULL");
	
	/* QueryPrepared - INSERT - NO Result */
	vtm_dataset_clear(bind);
	vtm_dataset_set_int(bind, "num", 30);
	vtm_dataset_set_string(bind, "msg", "DEF");
	rc = vtm_sql_query_prepared(con, SQL_INS_PREP, bind, &result);
	VTM_TEST_CHECK(rc != VTM_OK, "sql insert query-prepared error expected");
	
	/* QueryPrepared - SELECT */
	vtm_dataset_clear(bind);
	vtm_dataset_set_int(bind, "p1", 4);
	vtm_dataset_set_int(bind, "p2", 20);
	rc = vtm_sql_query_prepared(con,
		"SELECT msg as xy, ts_update"
		" FROM users WHERE user_id < :p1 and number <= :p2", bind, &result);
	VTM_TEST_ASSERT(rc == VTM_OK, "sql query-prepared");

	row_count = 0;
	row = vtm_dataset_new();
	VTM_TEST_ASSERT(row != NULL, "sql row");
	while(vtm_sql_result_fetch_row(&result, row) == VTM_OK)
		row_count++;
	
	VTM_TEST_CHECK(row_count == 3, "sql query-prepared check result size");
	vtm_dataset_free(row);
	vtm_sql_result_release(&result);
	
	/* Transaction Test */
	rc = vtm_sql_set_auto_commit(con, false);
	VTM_TEST_CHECK(rc == VTM_OK, "sql autocommit off");
	rc = vtm_sql_execute(con, SQL_INSERT);
	VTM_TEST_CHECK(rc == VTM_OK, "sql insert in transaction");
	rc = vtm_sql_rollback(con);
	VTM_TEST_CHECK(rc == VTM_OK, "sql rollback");
	rc = vtm_sql_execute(con, SQL_INSERT);
	VTM_TEST_CHECK(rc == VTM_OK, "sql insert2 in transaction");
	rc = vtm_sql_commit(con);
	VTM_TEST_CHECK(rc == VTM_OK, "sql commit");
	rc = vtm_sql_set_auto_commit(con, true);
	VTM_TEST_CHECK(rc == VTM_OK, "sql autocommit on");
	
	/* Query - SELECT */
	rc = vtm_sql_query(con, "SELECT * FROM users WHERE msg IS NULL", &result);
	VTM_TEST_ASSERT(rc == VTM_OK, "sql query");

	rc = vtm_sql_result_fetch_all(&result);
	VTM_TEST_CHECK(rc == VTM_OK, "sql query - result fetch all");
	
	VTM_TEST_CHECK(result.row_count == 3, "sql query - check result size");
	vtm_sql_result_release(&result);
}

static void test_sql_stmt(vtm_sql_con *con, vtm_dataset *bind)
{
	int rc;
	struct vtm_sql_stmt stmt;
	struct vtm_sql_result result;

	/* prepare stmt */
	rc = vtm_sql_prepare(con, SQL_INS_PREP, &stmt);
	VTM_TEST_ASSERT(rc == VTM_OK, "sql prepare statement");

	/* bind param */
	vtm_dataset_set_int(bind, "num", 500);
	vtm_dataset_set_string(bind, "msg", "ABC");

	rc = vtm_sql_stmt_bind(&stmt, bind);
	VTM_TEST_ASSERT(rc == VTM_OK, "sql stmt bind");

	/* execute */
	rc = vtm_sql_stmt_execute(&stmt);
	VTM_TEST_CHECK(rc == VTM_OK, "sql stmt execute");

	/* query - error expected */
	rc = vtm_sql_stmt_query(&stmt, &result);
	VTM_TEST_CHECK(rc != VTM_OK, "sql stmt query with no result");
	vtm_sql_stmt_release(&stmt);

	/* prepare for query */
	rc = vtm_sql_prepare(con, "SELECT number FROM users WHERE number >= :num",
	                     &stmt);

	/* bind param */
	vtm_dataset_clear(bind);
	vtm_dataset_set_int(bind, "num", 500);
	rc = vtm_sql_stmt_bind(&stmt, bind);
	VTM_TEST_ASSERT(rc == VTM_OK, "sql stmt bind");

	/* query */
	rc = vtm_sql_stmt_query(&stmt, &result);
	VTM_TEST_CHECK(rc == VTM_OK, "sql stmt query");

	/* fetch all rows */
	rc = vtm_sql_result_fetch_all(&result);
	VTM_TEST_ASSERT(rc == VTM_OK, "sql stmt fetch all");

	/* check correct column and row counts */
	VTM_TEST_CHECK(result.col_count == 1, "sql result col count");
	VTM_TEST_CHECK(result.row_count == 2, "sql result row count");

	/* check column names */
	rc = strcmp(result.columns[0], "number");
	VTM_TEST_CHECK(rc == 0, "sql result col names");

	/* check row values */
	VTM_TEST_CHECK(vtm_dataset_get_int(&result.rows[0], "number") == 500,
	               "sql result row value");

	vtm_sql_result_release(&result);
	vtm_sql_stmt_release(&stmt);
}

void test_sql_generic(vtm_sql_con *con)
{
	vtm_dataset *bind;

	/* create bind */
	bind = vtm_dataset_new();
	VTM_TEST_ASSERT(bind != NULL, "sql param bind");
		
	/* run tests */
	test_sql_simple(con, bind);
	test_sql_stmt(con, bind);

	/* free bind */
	vtm_dataset_free(bind);
	
	/* free connection */
	vtm_sql_con_free(con);
	VTM_TEST_PASSED("sql con free");
}
