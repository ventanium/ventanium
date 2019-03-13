/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "sql_statement.h"

#include <vtm/sql/sql_error.h>

#define VTM_SQL_STMT_CHECK_STATE(STMT)                        \
	do {                                                      \
		if ((STMT)->state == VTM_SQL_STMT_STATE_OPEN_RESULT)  \
			return VTM_E_SQL_PENDING_RESULT;                  \
	} while (0)

int vtm_sql_stmt_bind(struct vtm_sql_stmt *stmt, vtm_dataset *bind)
{
	VTM_SQL_STMT_CHECK_STATE(stmt);
	return stmt->fn_bind(stmt, bind);
}

int vtm_sql_stmt_execute(struct vtm_sql_stmt *stmt)
{
	VTM_SQL_STMT_CHECK_STATE(stmt);
	return stmt->fn_execute(stmt);
}

int vtm_sql_stmt_query(struct vtm_sql_stmt *stmt, struct vtm_sql_result *result)
{
	VTM_SQL_STMT_CHECK_STATE(stmt);
	return stmt->fn_query(stmt, result);
}

void vtm_sql_stmt_release(struct vtm_sql_stmt *stmt)
{
	stmt->fn_release(stmt);
}
