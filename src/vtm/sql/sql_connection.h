/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file sql_connection.h
 *
 * @brief Database connection
 */

#ifndef VTM_SQL_SQL_CONNECTION_H_
#define VTM_SQL_SQL_CONNECTION_H_

#include <vtm/core/api.h>
#include <vtm/core/dataset.h>
#include <vtm/core/types.h>
#include <vtm/sql/sql_error.h>
#include <vtm/sql/sql_result.h>
#include <vtm/sql/sql_statement.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtm_sql_con_vt vtm_sql_con;

VTM_API extern const char* const VTM_SQL_PAR_HOST;
VTM_API extern const char* const VTM_SQL_PAR_PORT;
VTM_API extern const char* const VTM_SQL_PAR_USER;
VTM_API extern const char* const VTM_SQL_PAR_PASSWORD;
VTM_API extern const char* const VTM_SQL_PAR_DATABASE;

/**
 * Sets the amount of time that functions wait for a locked resource before
 * returning an error.
 *
 * @param con the connection
 * @param millis the the timeout value in milliseconds
 * @return VTM_OK if the call succeed
 * @return VTM_E_NOT_SUPPORTED if the underlying database system does not
 *         support this feature
 * @return VTM_E_SQL_UNKNOWN or VTM_ERROR if an unknown error occured
 */
VTM_API int vtm_sql_set_lock_wait_timeout(vtm_sql_con *con, unsigned long millis);

/**
 * Set auto commit option on the connection.
 *
 * If auto commit is disabled the connection works in transaction mode,
 * where a bunch of related SQL statements can be commited or rollbacked
 * together.
 *
 * @param con the connection
 * @param commit true if auto-commit should be enabled
 * @return VTM_OK if the call succeed
 * @return VTM_E_SQL_PENDING_RESULT if there is an open resultset from this
 *         connection that has not been processed yet
 * @return VTM_E_SQL_UNKNOWN or VTM_ERROR if an unknown error occured
 */
VTM_API int vtm_sql_set_auto_commit(vtm_sql_con *con, bool commit);

/**
 * Execute commit on the given connection.
 *
 * That makes all changes since the last commit/rollback permanent.
 *
 * @param con the connection
 * @return VTM_OK if the call succeed
 * @return VTM_E_SQL_PENDING_RESULT if there is an open resultset from this
 *         connection that has not been processed yet
 * @return VTM_E_SQL_UNKNOWN or VTM_ERROR if an unknown error occured
 */
VTM_API int vtm_sql_commit(vtm_sql_con *con);

/**
 * Execute rollback on the given connection.
 *
 * That reverts all pending changes since the last commit/rollback.
 *
 * @param con the connection
 * @return VTM_OK if the call succeed
 * @return VTM_E_SQL_PENDING_RESULT if there is an open resultset from this
 *         connection that has not been processed yet
 * @return VTM_E_SQL_UNKNOWN or VTM_ERROR if an unknown error occured
 */
VTM_API int vtm_sql_rollback(vtm_sql_con *con);

/**
 * Creates a new prepared statment.
 *
 * The parameters which should be used by the statement are specified in the
 * given query string with ":<ParameterName>".
 *
 * @param con the connection
 * @param query the query string which is executed by the statement
 * @param stmt the variable that should hold the initialized statement
 * @return VTM_OK if the prepared statement was successfully created
 * @return VTM_E_SQL_PENDING_RESULT if there is an open resultset from this
 *         connection that has not been processed yet
 * @return VTM_E_SQL_UNKNOWN or VTM_ERROR if an unknown error occured
 */
VTM_API int vtm_sql_prepare(vtm_sql_con *con, const char *query, struct vtm_sql_stmt *stmt);

/**
 * Executes given SQL statement.
 *
 * @param con the connection
 * @param query the SQL statement that should executed
 * @return VTM_OK if the statement was successfully executed
 * @return VTM_E_SQL_PENDING_RESULT if there is an open resultset from this
 *         connection that has not been processed yet
 * @return VTM_E_SQL_UNKNOWN or VTM_ERROR if an unknown error occured
 */
VTM_API int vtm_sql_execute(vtm_sql_con *con, const char *query);

/**
 * Convenience method for executing a prepared statement once.
 *
 * @param con the connection
 * @param query the SQL statement containing parameters
 * @param bind the dataset containing the parameter values
 * @return VTM_OK if the statement was successfully executed
 * @return VTM_E_SQL_PENDING_RESULT if there is an open resultset from this
 *         connection that has not been processed yet
 * @return VTM_E_SQL_UNKNOWN or VTM_ERROR if an unknown error occured
 */
VTM_API int vtm_sql_execute_prepared(vtm_sql_con *con, const char *query, vtm_dataset *bind);

/**
 * Executes an SQL statement that returns a result.
 *
 * @param con the connection
 * @param query the SQL query that should executed
 * @param result the variable that should be initialized with the result set
 * @return VTM_OK if the query was successfully executed
 * @return VTM_E_SQL_PENDING_RESULT if there is an open resultset from this
 *         connection that has not been processed yet
 * @return VTM_E_SQL_UNKNOWN or VTM_ERROR if an unknown error occured
 */
VTM_API int vtm_sql_query(vtm_sql_con *con, const char *query, struct vtm_sql_result *result);

/**
 * Convenience method for querying a prepared statement once.
 *
 * @param con the connection
 * @param query the SQL statement containing parameters
 * @param bind the dataset containing the parameter values
 * @param result the variable that should be initialized with the result set
 * @return VTM_OK if the statement was successfully executed
 * @return VTM_E_SQL_PENDING_RESULT if there is an open resultset from this
 *         connection that has not been processed yet
 * @return VTM_E_SQL_UNKNOWN or VTM_ERROR if an unknown error occured
 */
VTM_API int vtm_sql_query_prepared(vtm_sql_con *con, const char *query, vtm_dataset *bind, struct vtm_sql_result *result);

/**
 * Closes the database connection and releases all associated resources.
 *
 * After this call the connection pointer is no longer valid.
 *
 * @param con the connection that should be released
 */
VTM_API void vtm_sql_con_free(vtm_sql_con *con);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_SQL_CONNECTION_H_ */
