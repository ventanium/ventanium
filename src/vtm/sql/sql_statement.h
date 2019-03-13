/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file sql_statement.h
 *
 * @brief Prepared statement
 */

#ifndef VTM_SQL_SQL_STATEMENT_H_
#define VTM_SQL_SQL_STATEMENT_H_

#include <vtm/core/api.h>
#include <vtm/core/dataset.h>
#include <vtm/sql/sql_result.h>

#ifdef __cplusplus
extern "C" {
#endif

enum vtm_sql_stmt_state
{
	VTM_SQL_STMT_STATE_READY,
	VTM_SQL_STMT_STATE_OPEN_RESULT
};

struct vtm_sql_stmt
{
	void *stmt_data;
	enum vtm_sql_stmt_state state;

	int (*fn_bind)(struct vtm_sql_stmt *stmt, vtm_dataset *bind);
	int (*fn_execute)(struct vtm_sql_stmt *stmt);
	int (*fn_query)(struct vtm_sql_stmt *stmt, struct vtm_sql_result *result);
	void (*fn_release)(struct vtm_sql_stmt *stmt);
};

/**
 * Bind parameters to statement.
 *
 * @param stmt the statement where the parameters should be bound to
 * @param bind the dataset containing the parameter values
 * @return VTM_OK if parameter successfully bound
 * @return VTM_E_SQL_PENDING_RESULT if there is an open resultset from this
 *         statement that has not been processed yet
 * @return VTM_E_SQL_UNKOWN or VTM_ERROR if an unknown error occured
 */
VTM_API int vtm_sql_stmt_bind(struct vtm_sql_stmt *stmt, vtm_dataset *bind);

/**
 * Executes the statement.
 *
 * @param stmt the statement that should be executed
 * @return VTM_OK if the statment was successfully executed
 * @return VTM_E_SQL_PENDING_RESULT if there is an open resultset from this
 *         statement that has not been processed yet
 * @return VTM_E_SQL_UNKOWN or VTM_ERROR if an unknown error occured
 */
VTM_API int vtm_sql_stmt_execute(struct vtm_sql_stmt *stmt);

/**
 * Executes the statement and returns a result set.
 *
 * @param stmt the statement that should be executed
 * @param result result set that should be initialized
 * @return VTM_OK if the statment was successfully executed
 * @return VTM_E_SQL_PENDING_RESULT if there is already an open resultset from
 *         this statement that has not been processed yet
 * @return VTM_E_SQL_NO_RESULT if the executed statement does not return rows
 *         for example an INSERT or UPDATE statement
 * @return VTM_E_SQL_UNKOWN or VTM_ERROR if an unknown error occured
 */
VTM_API int vtm_sql_stmt_query(struct vtm_sql_stmt *stmt, struct vtm_sql_result *result);

/**
 * Releases the statement.
 *
 * After this call the statement is no longer valid for use in other functions.
 *
 * @param stmt the statement that should be released
 */
VTM_API void vtm_sql_stmt_release(struct vtm_sql_stmt *stmt);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_SQL_STATEMENT_H_ */
