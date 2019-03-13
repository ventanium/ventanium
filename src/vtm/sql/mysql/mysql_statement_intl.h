/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_SQL_MYSQL_MYSQL_STATEMENT_INTL_H_
#define VTM_SQL_MYSQL_MYSQL_STATEMENT_INTL_H_

#include <mysql.h>
#include <vtm/sql/sql_statement.h>

#ifdef __cplusplus
extern "C" {
#endif

int vtm_mysql_stmt_prepare(struct vtm_sql_stmt *stmt, MYSQL *con, const char *query);
void vtm_mysql_stmt_release(struct vtm_sql_stmt *stmt);

int vtm_mysql_stmt_bind(struct vtm_sql_stmt *stmt, vtm_dataset *bind);
int vtm_mysql_stmt_execute(struct vtm_sql_stmt *stmt);
int vtm_mysql_stmt_query(struct vtm_sql_stmt *stmt, struct vtm_sql_result *result);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_MYSQL_MYSQL_STATEMENT_INTL_H_ */
