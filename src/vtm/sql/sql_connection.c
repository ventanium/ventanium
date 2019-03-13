/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "sql_connection.h"

#include <stdlib.h> /* free() */
#include <vtm/core/error.h>
#include <vtm/core/lang.h>
#include <vtm/sql/sql_connection_intl.h>

#define VTM_SQL_CON_CHECK_STATE(CON)                        \
	do {                                                    \
		if ((CON)->state == VTM_SQL_CON_STATE_OPEN_RESULT)  \
			return VTM_E_SQL_PENDING_RESULT;                \
	} while (0)

const char* const VTM_SQL_PAR_HOST = "HOST";
const char* const VTM_SQL_PAR_PORT = "PORT";
const char* const VTM_SQL_PAR_USER = "USER";
const char* const VTM_SQL_PAR_PASSWORD = "PASSWORD";
const char* const VTM_SQL_PAR_DATABASE = "DATABASE";

int vtm_sql_set_auto_commit(vtm_sql_con *con, bool commit)
{
	VTM_SQL_CON_CHECK_STATE(con);
	return con->fn_set_auto_commit(con, commit);
}

int vtm_sql_commit(vtm_sql_con *con)
{
	VTM_SQL_CON_CHECK_STATE(con);
	return con->fn_commit(con);
}

int vtm_sql_rollback(vtm_sql_con *con)
{
	VTM_SQL_CON_CHECK_STATE(con);
	return con->fn_rollback(con);
}

int vtm_sql_prepare(vtm_sql_con *con, const char *query, struct vtm_sql_stmt *stmt)
{
	VTM_SQL_CON_CHECK_STATE(con);
	return con->fn_prepare(con, query, stmt);
}

int vtm_sql_execute(vtm_sql_con *con, const char *query)
{
	VTM_SQL_CON_CHECK_STATE(con);
	return con->fn_execute(con, query);
}

int vtm_sql_execute_prepared(vtm_sql_con *con, const char *query, vtm_dataset *bind)
{
	int rc;
	struct vtm_sql_stmt stmt;
	
	VTM_SQL_CON_CHECK_STATE(con);

	rc = con->fn_prepare(con, query, &stmt);
	if (rc != VTM_OK)
		return rc;

	rc = vtm_sql_stmt_bind(&stmt, bind);
	if (rc == VTM_OK)
		rc = vtm_sql_stmt_execute(&stmt);

	vtm_sql_stmt_release(&stmt);
	
	return rc;
}

int vtm_sql_query(vtm_sql_con *con, const char *query, struct vtm_sql_result *result)
{
	VTM_SQL_CON_CHECK_STATE(con);
	return con->fn_query(con, query, result);
}

int vtm_sql_query_prepared(vtm_sql_con *con, const char *query, vtm_dataset *bind, struct vtm_sql_result *result)
{
	int rc;
	struct vtm_sql_stmt *stmt;
	
	VTM_SQL_CON_CHECK_STATE(con);

	stmt = malloc(sizeof(*stmt));
	if (!stmt) {
		vtm_err_oom();
		return vtm_err_get_code();
	}
	
	rc = con->fn_prepare(con, query, stmt);
	if (rc != VTM_OK)
		return rc;

	rc = vtm_sql_stmt_bind(stmt, bind);
	if (rc != VTM_OK)
		goto end;

	rc = vtm_sql_stmt_query(stmt, result);
	if (rc != VTM_OK)
		goto end;

	result->fn_release_owner = vtm_sql_stmt_free;
	
end:
	if (rc != VTM_OK)
		vtm_sql_stmt_free(stmt);

	return rc;
}

void vtm_sql_con_free(vtm_sql_con *con)
{
	if (!con)
		return;
	
	con->fn_free(con);
	free(con);
}

void vtm_sql_stmt_free(void *stmt)
{
	vtm_sql_stmt_release(stmt);
	free(stmt);
}
