/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_SQL_SQLITE_SQLITE_STATEMENT_INTL_H_
#define VTM_SQL_SQLITE_SQLITE_STATEMENT_INTL_H_

#include <sqlite3.h>
#include <vtm/sql/sql_statement.h>

#ifdef __cplusplus
extern "C" {
#endif

int vtm_sqlite_stmt_prepare(struct vtm_sql_stmt *stmt, sqlite3 *con, const char *query);
void vtm_sqlite_stmt_release(struct vtm_sql_stmt *stmt);

int vtm_sqlite_stmt_bind(struct vtm_sql_stmt *stmt, vtm_dataset *bind);
int vtm_sqlite_stmt_execute(struct vtm_sql_stmt *stmt);
int vtm_sqlite_stmt_query(struct vtm_sql_stmt *stmt, struct vtm_sql_result *result);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_SQLITE_SQLITE_STATEMENT_INTL_H_ */
