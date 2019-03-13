/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "mysql_statement_intl.h"

#include <stdlib.h> /* malloc(), free() */
#include <mysql.h>

#include <vtm/core/buffer.h>
#include <vtm/core/error.h>
#include <vtm/core/list.h>
#include <vtm/core/string.h>
#include <vtm/core/types.h>
#include <vtm/sql/sql_error.h>
#include <vtm/sql/sql_result_intl.h>
#include <vtm/sql/sql_util_intl.h>
#include <vtm/sql/mysql/mysql_bind_intl.h>
#include <vtm/sql/mysql/mysql_error_intl.h>

struct vtm_mysql_stmt
{
	MYSQL_STMT *stmt;
	vtm_list *param_names;
	struct vtm_mysql_bind param_bind;
	struct vtm_mysql_bind result_bind;
};

/* forward declaration */
static int  vtm_mysql_stmt_result_fetch_row(struct vtm_sql_result *res, vtm_dataset *row);
static int  vtm_mysql_stmt_result_fetch_all(struct vtm_sql_result *res);
static void vtm_mysql_stmt_result_finish(struct vtm_sql_result *res);

int vtm_mysql_stmt_prepare(struct vtm_sql_stmt *stmt, MYSQL *con, const char *query)
{
	int rc;
	struct vtm_mysql_stmt *stmt_data;
	struct vtm_buf query_buf;
	unsigned long param_count;

	stmt_data = malloc(sizeof(*stmt_data));
	if (!stmt_data) {
		vtm_err_oom();
		return vtm_err_get_code();
	}

	/* create mysql stmt */
	stmt_data->stmt = mysql_stmt_init(con);
	if (!stmt_data->stmt) {
		free(stmt_data);
		return vtm_err_set(VTM_ERROR);
	}

	/* create list for param names */
	stmt_data->param_names = vtm_list_new(VTM_ELEM_STRING, 8);
	if (!stmt_data->param_names) {
		mysql_stmt_close(stmt_data->stmt);
		free(stmt_data);
		return vtm_err_get_code();
	}

	/* extract param names */
	vtm_buf_init(&query_buf, VTM_BYTEORDER_LE);
	rc = vtm_sql_extract_query_params(&query_buf, query, stmt_data->param_names);
	if (rc != VTM_OK) {
		mysql_stmt_close(stmt_data->stmt);
		vtm_list_free(stmt_data->param_names);
		vtm_buf_release(&query_buf);
		free(stmt_data);
		return rc;
	}

	/* prepare statement */
	rc = mysql_stmt_prepare(stmt_data->stmt, (char*) query_buf.data, query_buf.used);
	vtm_buf_release(&query_buf);
	if (rc != 0) {
		rc = vtm_mysql_error_stmt(stmt_data->stmt);
		mysql_stmt_close(stmt_data->stmt);
		vtm_list_free(stmt_data->param_names);
		free(stmt_data);
		return rc;
	}

	/* check param count */
	param_count = mysql_stmt_param_count(stmt_data->stmt);
	if (vtm_list_size(stmt_data->param_names) != param_count) {
		mysql_stmt_close(stmt_data->stmt);
		vtm_list_free(stmt_data->param_names);
		free(stmt_data);
		return vtm_err_set(VTM_E_SQL_PARAMCOUNT);
	}

	/* init param bind */
	rc = vtm_mysql_bind_init(&stmt_data->param_bind, (size_t) param_count);
	if (rc != VTM_OK) {
		mysql_stmt_close(stmt_data->stmt);
		vtm_list_free(stmt_data->param_names);
		free(stmt_data);
		return rc;
	}

	/* init statement */
	stmt->state = VTM_SQL_STMT_STATE_READY;
	stmt->stmt_data = stmt_data;

	stmt->fn_release = vtm_mysql_stmt_release;
	stmt->fn_bind = vtm_mysql_stmt_bind;
	stmt->fn_execute = vtm_mysql_stmt_execute;
	stmt->fn_query = vtm_mysql_stmt_query;

	return VTM_OK;
}

void vtm_mysql_stmt_release(struct vtm_sql_stmt *stmt)
{
	struct vtm_mysql_stmt *stmt_data;

	stmt_data = stmt->stmt_data;

	vtm_mysql_bind_release(&stmt_data->param_bind);
	mysql_stmt_close(stmt_data->stmt);
	vtm_list_free(stmt_data->param_names);
	free(stmt_data);
}

int vtm_mysql_stmt_bind(struct vtm_sql_stmt *stmt, vtm_dataset *bind)
{
	int rc;
	struct vtm_mysql_stmt* stmt_data;

	stmt_data = stmt->stmt_data;

	rc = vtm_mysql_bind_write(&stmt_data->param_bind, bind,
		                      stmt_data->param_names);
	if (rc != VTM_OK)
		return rc;

	rc = mysql_stmt_bind_param(stmt_data->stmt, stmt_data->param_bind.bindings);
	if (rc != 0)
		return vtm_mysql_error_stmt(stmt_data->stmt);

	return VTM_OK;
}

int vtm_mysql_stmt_execute(struct vtm_sql_stmt *stmt)
{
	MYSQL_STMT *my_stmt;

	my_stmt = ((struct vtm_mysql_stmt*) stmt->stmt_data)->stmt;

	if (mysql_stmt_execute(my_stmt) != 0)
		return vtm_mysql_error_stmt(my_stmt);

	return VTM_OK;
}

int vtm_mysql_stmt_query(struct vtm_sql_stmt *stmt, struct vtm_sql_result *result)
{
	int rc;
	MYSQL_STMT *my_stmt;
	MYSQL_RES *meta;
	MYSQL_FIELD *fields;
	struct vtm_mysql_stmt *stmt_data;
	struct vtm_mysql_bind *result_bind;
	unsigned int i, field_count;

	stmt_data = stmt->stmt_data;
	my_stmt = stmt_data->stmt;
	result_bind = &stmt_data->result_bind;

	/* execute */
	if (mysql_stmt_execute(my_stmt) != 0)
		return vtm_mysql_error_stmt(my_stmt);

	/* prepare result bind */
	meta = mysql_stmt_result_metadata(my_stmt);
	if (!meta)
		return vtm_err_set(VTM_E_SQL_NO_RESULT);

	vtm_sql_result_init(result);

	field_count = mysql_num_fields(meta);
	rc = vtm_mysql_bind_init(result_bind, field_count);
	if (rc != VTM_OK)
		goto end;

	fields = mysql_fetch_fields(meta);
	result->columns = calloc(field_count, sizeof(const char*));
	if (!result->columns) {
		vtm_err_oom();
		rc = vtm_err_get_code();
		goto end;
	}

	/* create bind buffers */
	result->col_count = field_count;
	for (i=0; i < field_count; i++) {
		result->columns[i] = vtm_str_copy((fields+i)->name);
		if (!result->columns[i]) {
			rc = vtm_err_get_code();
			goto end;
		}

		rc = vtm_mysql_bind_buf_init_from_field(&result_bind->buffers[i],
			                                    fields+i);
		if (rc != VTM_OK)
			goto end;

		vtm_mysql_bind_buf_couple(&result_bind->buffers[i],
			                      &result_bind->bindings[i]);
	}

	/* bind buffers to mysql statement */
	rc = mysql_stmt_bind_result(my_stmt, result_bind->bindings);
	if (rc != 0) {
		rc = vtm_mysql_error_stmt(my_stmt);
		goto end;
	}

	/* set callbacks */
	result->fn_fetch_row = vtm_mysql_stmt_result_fetch_row;
	result->fn_fetch_all = vtm_mysql_stmt_result_fetch_all;
	result->fn_finish = vtm_mysql_stmt_result_finish;
	result->fn_release = vtm_sql_result_release_default;

	result->res_data = stmt_data;
	result->res_owner = stmt;

	stmt->state = VTM_SQL_STMT_STATE_OPEN_RESULT;
	rc = VTM_OK;

end:
	if (rc != VTM_OK) {
		vtm_sql_result_release_default(result);
		vtm_mysql_bind_release(result_bind);
	}

	if (meta)
		mysql_free_result(meta);

	return rc;
}

static int vtm_mysql_stmt_result_fetch_row(struct vtm_sql_result *res, vtm_dataset *row)
{
	int rc;
	struct vtm_mysql_stmt *stmt_data;

	stmt_data = res->res_data;
	if (!stmt_data)
		return VTM_E_INVALID_STATE;

	rc = mysql_stmt_fetch(stmt_data->stmt);
	if (rc != 0) {
		return (rc == MYSQL_NO_DATA)
			? VTM_E_SQL_RESULT_COMPLETE
			: vtm_mysql_error_stmt(stmt_data->stmt);
	}

	vtm_dataset_clear(row);

	return vtm_mysql_bind_read(&stmt_data->result_bind, row);
}

static int vtm_mysql_stmt_result_fetch_all(struct vtm_sql_result *res)
{
	int rc;
	struct vtm_mysql_stmt *stmt_data;
	MYSQL_STMT *my_stmt;
	my_ulonglong row_count;

	stmt_data = res->res_data;
	if (!stmt_data)
		return VTM_E_INVALID_STATE;

	my_stmt = stmt_data->stmt;

	/* store complete result */
	rc = mysql_stmt_store_result(my_stmt);
	if (rc != 0) {
		vtm_mysql_error_stmt(my_stmt);
		rc = VTM_E_SQL_STORE_RESULT;
		goto end;
	}

	/* determine number of rows */
	row_count = mysql_stmt_num_rows(my_stmt);
	if (row_count > SIZE_MAX) {
		rc = vtm_err_set(VTM_E_OVERFLOW);
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
	while (mysql_stmt_fetch(my_stmt) == 0) {
		vtm_dataset_init(&res->rows[res->row_count], VTM_DS_HINT_DEFAULT);
		rc = vtm_mysql_bind_read(&stmt_data->result_bind,
		                         &res->rows[res->row_count]);

		/* make sure release has already correct row count */
		res->row_count++;
		if (rc != VTM_OK)
			goto end;
	}

	rc = VTM_OK;

end:

	return rc;
}

static void vtm_mysql_stmt_result_finish(struct vtm_sql_result *res)
{
	struct vtm_sql_stmt *stmt;
	struct vtm_mysql_stmt *stmt_data;

	stmt = res->res_owner;
	stmt_data = stmt->stmt_data;

	vtm_mysql_bind_release(&stmt_data->result_bind);
	stmt->state = VTM_SQL_STMT_STATE_READY;
}
