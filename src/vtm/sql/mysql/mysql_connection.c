/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "mysql_connection_intl.h"

#include <stdlib.h>
#include <string.h> /* memset() */
#include <mysql.h>
#include <errmsg.h>

#include <vtm/core/buffer.h>
#include <vtm/core/error.h>
#include <vtm/core/math.h>
#include <vtm/core/string.h>
#include <vtm/sql/sql_connection_intl.h>
#include <vtm/sql/sql_result_intl.h>
#include <vtm/sql/mysql/mysql_error_intl.h>
#include <vtm/sql/mysql/mysql_statement_intl.h>

#define VTM_MYSQL_CON(con) (MYSQL*)(con)->con_data

/* forward declaration */
static int vtm_mysql_connect(vtm_sql_con *con, vtm_dataset *param);
static int vtm_mysql_free(vtm_sql_con *con);
static int vtm_mysql_set_auto_commit(vtm_sql_con *con, bool commit);
static int vtm_mysql_commit(vtm_sql_con *con);
static int vtm_mysql_rollback(vtm_sql_con *con);
static int vtm_mysql_prepare(vtm_sql_con *con, const char *query, struct vtm_sql_stmt *stmt);
static int vtm_mysql_execute(vtm_sql_con *con, const char *query);
static int vtm_mysql_query(vtm_sql_con *con, const char *query, struct vtm_sql_result *result);
static int vtm_mysql_save_result_columns(MYSQL_RES *result, struct vtm_sql_result *res);
static int vtm_mysql_result_fetch_row(struct vtm_sql_result *res, vtm_dataset *row);
static int vtm_mysql_result_fetch_all(struct vtm_sql_result *res);
static void vtm_mysql_result_finish(struct vtm_sql_result *res);
static void vtm_mysql_result_release(struct vtm_sql_result *res);

vtm_sql_con* vtm_mysql_con_new(vtm_dataset *param)
{
	vtm_sql_con *result;

	result = malloc(sizeof(vtm_sql_con));
	if (!result) {
		vtm_err_oom();
		return NULL;
	}

	if (vtm_mysql_connect(result, param) != VTM_OK) {
		free(result);
		return NULL;
	}

	result->fn_set_auto_commit = vtm_mysql_set_auto_commit;
	result->fn_commit = vtm_mysql_commit;
	result->fn_rollback = vtm_mysql_rollback;
	result->fn_prepare = vtm_mysql_prepare;
	result->fn_execute = vtm_mysql_execute;
	result->fn_query = vtm_mysql_query;
	result->fn_free = vtm_mysql_free;
	
	result->state = VTM_SQL_CON_STATE_READY;
	
	return result;
}

static int vtm_mysql_free(vtm_sql_con *con)
{
	mysql_close(VTM_MYSQL_CON(con));
	return VTM_OK;
}

static int vtm_mysql_connect(vtm_sql_con *con, vtm_dataset *param)
{
	MYSQL *my;
	const char* host = vtm_dataset_get_string(param, VTM_SQL_PAR_HOST);
	const char* user = vtm_dataset_get_string(param, VTM_SQL_PAR_USER);
	const char* pw = vtm_dataset_get_string(param, VTM_SQL_PAR_PASSWORD);
	const char* db = vtm_dataset_get_string(param, VTM_SQL_PAR_DATABASE);
	const int port = vtm_dataset_get_int(param, VTM_SQL_PAR_PORT);

	my = mysql_init(NULL);

	if (mysql_real_connect(my, host, user, pw, db, port, NULL, 0) != my) {
		vtm_mysql_error(my);
		mysql_close(my);
		return vtm_err_get_code();
	}

	con->con_data = my;

	return VTM_OK;
}

static int vtm_mysql_set_auto_commit(vtm_sql_con *con, bool commit)
{
	my_bool val;

	val = commit ? 1 : 0;

	if (mysql_autocommit(VTM_MYSQL_CON(con), val) != 0)
		return vtm_mysql_error(VTM_MYSQL_CON(con));

	return VTM_OK;
}

static int vtm_mysql_commit(vtm_sql_con *con)
{
	if (mysql_commit(VTM_MYSQL_CON(con)) != 0)
		return vtm_mysql_error(VTM_MYSQL_CON(con));

	return VTM_OK;
}

static int vtm_mysql_rollback(vtm_sql_con *con)
{
	if (mysql_rollback(VTM_MYSQL_CON(con)) != 0)
		return vtm_mysql_error(VTM_MYSQL_CON(con));

	return VTM_OK;
}

static int vtm_mysql_prepare(vtm_sql_con *con, const char *query, struct vtm_sql_stmt *stmt)
{
	return vtm_mysql_stmt_prepare(stmt, VTM_MYSQL_CON(con), query);
}

static int vtm_mysql_execute(vtm_sql_con *con, const char *query)
{
	if (mysql_query(VTM_MYSQL_CON(con), query) != 0)
		return vtm_mysql_error(VTM_MYSQL_CON(con));

	return VTM_OK;
}

static int vtm_mysql_query(vtm_sql_con *con, const char *query, struct vtm_sql_result *result)
{
	int rc;

	/* execute */
	rc = vtm_mysql_execute(con, query);
	if (rc != VTM_OK)
		return rc;

	/* init result */
	vtm_sql_result_init(result);

	/* set callbacks */
	result->fn_fetch_row = vtm_mysql_result_fetch_row;
	result->fn_fetch_all = vtm_mysql_result_fetch_all;
	result->fn_finish = vtm_mysql_result_finish;
	result->fn_release = vtm_mysql_result_release;
	result->res_owner = con;

	con->state = VTM_SQL_CON_STATE_OPEN_RESULT;

	return VTM_OK;
}

static int vtm_mysql_save_result_columns(MYSQL_RES *result, struct vtm_sql_result *res)
{
	MYSQL_FIELD *fields;
	size_t i;

	res->col_count = mysql_num_fields(result);
	res->columns = calloc(res->col_count, sizeof(char**));
	if (!res->columns) {
		vtm_err_oom();
		return vtm_err_get_code();
	}
	
	fields = mysql_fetch_fields(result);

	for (i=0; i < res->col_count; i++) {
		res->columns[i] = vtm_str_copy(fields[i].name);
		if (!res->columns[i])
			return vtm_err_get_code();
	}

	return VTM_OK;
}

static int vtm_mysql_result_fetch_row(struct vtm_sql_result *res, vtm_dataset *row)
{
	int rc;
	vtm_sql_con *con;
	MYSQL *my_con;
	MYSQL_RES *my_res;
	MYSQL_ROW my_row;
	size_t i;

	con = res->res_owner;
	my_con = VTM_MYSQL_CON(con);

	my_res = res->res_data;
	if (!my_res) {
		my_res = mysql_use_result(my_con);
		if (!my_res) {
			rc = vtm_mysql_error(my_con);
			goto end;
		}
		res->res_data = my_res;

		/* extract result column names */
		rc = vtm_mysql_save_result_columns(my_res, res);
		if (rc != VTM_OK)
			goto end;
	}

	/* fetch next row */
	my_row = mysql_fetch_row(my_res);
	if (!my_row) {
		if (mysql_errno(my_con) == 0)
			rc = VTM_E_SQL_RESULT_COMPLETE;
		else 
			rc = vtm_mysql_error(my_con);
		goto end;	
	}

	/* write row contents to dataset */
	for (i=0; i < res->col_count; i++) {
		if (!my_row[i])
			continue;
		vtm_dataset_set_string(row, res->columns[i], my_row[i]);
	}

	rc = VTM_OK;

end:
	if (rc != VTM_OK && my_res)
		mysql_free_result(my_res);
	
	return rc;
}

static int vtm_mysql_result_fetch_all(struct vtm_sql_result *res)
{
	int rc;
	vtm_sql_con *con;
	MYSQL *my_con;
	MYSQL_RES *my_res;
	MYSQL_ROW my_row;
	my_ulonglong row_count;
	size_t i;

	con = res->res_owner;
	my_con = VTM_MYSQL_CON(con);

	/* store result */
	my_res = mysql_store_result(my_con);
	if (!my_res) {
		rc = vtm_mysql_error(my_con);
		goto end;
	}

	/* extract result column names */
	rc = vtm_mysql_save_result_columns(my_res, res);
	if (rc != VTM_OK)
		goto end;

	/* determine number of rows */
	row_count = mysql_num_rows(my_res);
	if (row_count > SIZE_MAX) {
		rc = VTM_E_OVERFLOW;
		goto end;
	}

	/* allocate rows */
	res->rows = calloc(row_count, sizeof(struct vtm_dataset));
	if (!res->rows) {
		vtm_err_oom();
		rc = vtm_err_get_code();
		goto end;
	}

	/* fetch rows */
	res->row_count = 0;
	while ((my_row = mysql_fetch_row(my_res))) {
		vtm_dataset_init(&res->rows[res->row_count], VTM_DS_HINT_DEFAULT);
		for (i=0; i < res->col_count; i++) {
			if (!my_row[i])
				continue;
			vtm_dataset_set_string(&res->rows[res->row_count],
			                       res->columns[i], my_row[i]);
		}
		res->row_count++;
	}

	rc = VTM_OK;

end:
	if (my_res)
		mysql_free_result(my_res);

	return rc;
}

static void vtm_mysql_result_finish(struct vtm_sql_result *res)
{
	vtm_sql_con *con;

	con = res->res_owner;
	con->state = VTM_SQL_CON_STATE_READY;
}

static void vtm_mysql_result_release(struct vtm_sql_result *res)
{
	MYSQL_RES *my_res;

	my_res = res->res_data;
	if (my_res)
		mysql_free_result(my_res);

	vtm_sql_result_release_default(res);
}
