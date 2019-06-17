/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "sqlite_connection_intl.h"

#include <stdlib.h>
#include <string.h> /* memset() */
#include <sqlite3.h>

#include <vtm/core/buffer.h>
#include <vtm/core/error.h>
#include <vtm/core/math.h>
#include <vtm/core/string.h>
#include <vtm/sql/sql_connection_intl.h>
#include <vtm/sql/sqlite/sqlite_statement_intl.h>

#define VTM_SQLITE_CON(con) (sqlite3*)(con)->con_data

/* forward declaration */
static int vtm_sqlite_connect(vtm_sql_con *con, vtm_dataset *param);
static int vtm_sqlite_free(vtm_sql_con *con);
static int vtm_sqlite_set_lock_wait_timeout(vtm_sql_con *con, unsigned long millis);
static int vtm_sqlite_set_auto_commit(vtm_sql_con *con, bool commit);
static int vtm_sqlite_commit(vtm_sql_con *con);
static int vtm_sqlite_rollback(vtm_sql_con *con);
static int vtm_sqlite_prepare(vtm_sql_con *con, const char *query, struct vtm_sql_stmt *stmt);
static int vtm_sqlite_execute(vtm_sql_con *con, const char *query);
static int vtm_sqlite_query(vtm_sql_con *con, const char *query, struct vtm_sql_result *result);

vtm_sql_con* vtm_sqlite_con_new(vtm_dataset *param)
{
	vtm_sql_con *result;

	result = malloc(sizeof(vtm_sql_con));
	if (!result) {
		vtm_err_oom();
		return NULL;
	}

	memset(result, 0, sizeof(vtm_sql_con));

	if (vtm_sqlite_connect(result, param) != VTM_OK) {
		free(result);
		return NULL;
	}

	result->fn_set_lock_wait_timeout = vtm_sqlite_set_lock_wait_timeout;
	result->fn_set_auto_commit = vtm_sqlite_set_auto_commit;
	result->fn_commit = vtm_sqlite_commit;
	result->fn_rollback = vtm_sqlite_rollback;
	result->fn_prepare = vtm_sqlite_prepare;
	result->fn_execute = vtm_sqlite_execute;
	result->fn_query = vtm_sqlite_query;
	result->fn_free = vtm_sqlite_free;

	return result;
}

static int vtm_sqlite_connect(vtm_sql_con *con, vtm_dataset *param)
{
	int rc;
	sqlite3 *db;

	const char* name = vtm_dataset_get_string(param, VTM_SQL_PAR_DATABASE);

	rc = sqlite3_open(name, &db);
	if (rc != SQLITE_OK)
		return VTM_ERROR;

	con->con_data = db;

	return VTM_OK;
}

static int vtm_sqlite_free(vtm_sql_con *con)
{
	if (sqlite3_close(VTM_SQLITE_CON(con)) != SQLITE_OK)
		return VTM_ERROR;
	return VTM_OK;
}

static int vtm_sqlite_set_lock_wait_timeout(vtm_sql_con *con, unsigned long millis)
{
	if (sqlite3_busy_timeout(VTM_SQLITE_CON(con), millis) != SQLITE_OK)
		return VTM_ERROR;
	return VTM_OK;
}

static int vtm_sqlite_set_auto_commit(vtm_sql_con *con, bool commit)
{
	return vtm_sqlite_execute(con, commit ? "COMMIT" : "BEGIN");
}

static int vtm_sqlite_commit(vtm_sql_con *con)
{
	int rc, auto_commit_enabled;

	auto_commit_enabled = sqlite3_get_autocommit(VTM_SQLITE_CON(con));

	rc = vtm_sql_execute(con, "COMMIT");
	if (rc != VTM_OK)
		return rc;

	if (!auto_commit_enabled)
		return vtm_sql_execute(con, "BEGIN");

	return VTM_OK;
}

static int vtm_sqlite_rollback(vtm_sql_con *con)
{
	int rc, auto_commit_enabled;

	auto_commit_enabled = sqlite3_get_autocommit(VTM_SQLITE_CON(con));

	rc = vtm_sql_execute(con, "ROLLBACK");
	if (rc != VTM_OK)
		return rc;

	if (!auto_commit_enabled)
		return vtm_sql_execute(con, "BEGIN");

	return VTM_OK;
}

static int vtm_sqlite_prepare(vtm_sql_con *con, const char *query, struct vtm_sql_stmt *stmt)
{
	return vtm_sqlite_stmt_prepare(stmt, VTM_SQLITE_CON(con), query);
}

static int vtm_sqlite_execute(vtm_sql_con *con, const char *query)
{
	int rc;
	char *err_msg;

	rc = sqlite3_exec(VTM_SQLITE_CON(con), query, NULL, NULL, &err_msg);
	if (rc != SQLITE_OK) {
		rc = vtm_err_sets(VTM_ERROR, err_msg);
		sqlite3_free(err_msg);
		return rc;
	}

	return VTM_OK;
}

static int vtm_sqlite_query(vtm_sql_con *con, const char *query, struct vtm_sql_result *result)
{
	int rc;
	struct vtm_sql_stmt *stmt;

	stmt = malloc(sizeof(*stmt));
	if (!stmt) {
		vtm_err_oom();
		return vtm_err_get_code();
	}

	rc = vtm_sqlite_prepare(con, query, stmt);
	if (rc != VTM_OK)
		return rc;

	rc = vtm_sql_stmt_query(stmt, result);
	if (rc != VTM_OK)
		goto end;

	result->fn_release_owner = vtm_sql_stmt_free;

end:
	if (rc != VTM_OK)
		vtm_sql_stmt_free(stmt);

	return rc;
}
