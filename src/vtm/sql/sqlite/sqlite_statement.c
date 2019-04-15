/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "sqlite_statement_intl.h"

#include <string.h> /* memcpy() */
#include <sqlite3.h>
#include <vtm/core/blob.h>
#include <vtm/core/error.h>
#include <vtm/core/string.h>
#include <vtm/sql/sql_error.h>
#include <vtm/sql/sql_result_intl.h>
#include <vtm/sql/sql_util_intl.h>
#include <vtm/sql/sqlite/sqlite_error_intl.h>

struct vtm_sqlite_stmt_data
{
	sqlite3 *con;
	sqlite3_stmt *stmt;
	vtm_list *param_names;

	int *column_types;
	int col_count;
	int last_step;
};

/* forward declaration */
static int  vtm_sqlite_stmt_bind_variant(sqlite3_stmt *stmt, int index, struct vtm_variant *var);
static int  vtm_sqlite_stmt_save_result_columns(struct vtm_sqlite_stmt_data *stmt_data, struct vtm_sql_result *res);
static int  vtm_sqlite_stmt_save_row_values(struct vtm_sqlite_stmt_data *stmt_data, struct vtm_sql_result *res, vtm_dataset *row);
static int  vtm_sqlite_stmt_result_fetch_row(struct vtm_sql_result *res, vtm_dataset *row);
static int  vtm_sqlite_stmt_result_fetch_all(struct vtm_sql_result *res);
static void vtm_sqlite_stmt_result_finish(struct vtm_sql_result *res);

int vtm_sqlite_stmt_prepare(struct vtm_sql_stmt *stmt, sqlite3 *con, const char *query)
{
	int rc;
	struct vtm_sqlite_stmt_data *stmt_data;
	struct vtm_buf query_buf;

	stmt_data = malloc(sizeof(*stmt_data));
	if (!stmt_data) {
		vtm_err_oom();
		return vtm_err_get_code();
	}

	/* create list for param names */
	stmt_data->param_names = vtm_list_new(VTM_ELEM_STRING, 8);
	if (!stmt_data->param_names) {
		rc = vtm_err_get_code();
		goto err_data;
	}

	/* extract param names */
	vtm_buf_init(&query_buf, VTM_BYTEORDER_LE);
	rc = vtm_sql_extract_query_params(&query_buf, query, stmt_data->param_names);
	if (rc != VTM_OK)
		goto err_buf;

	/* overflow check */
	if (vtm_list_size(stmt_data->param_names) > INT_MAX) {
		rc = VTM_E_OVERFLOW;
		goto err_buf;
	}

	if (query_buf.used > INT_MAX) {
		rc = VTM_E_OVERFLOW;
		goto err_buf;
	}

	/* prepare statement */
	rc = sqlite3_prepare(con, (char*) query_buf.data, query_buf.used,
	                     &stmt_data->stmt, NULL);
	if (rc != SQLITE_OK) {
		rc = vtm_sqlite_error(rc, con);
		goto err_buf;
	}
	vtm_buf_release(&query_buf);

	/* init statement data */
	stmt_data->con = con;
	stmt_data->column_types = NULL;
	stmt_data->col_count = 0;
	stmt_data->last_step = 0;

	/* init statement */
	stmt->state = VTM_SQL_STMT_STATE_READY;
	stmt->stmt_data = stmt_data;

	stmt->fn_release = vtm_sqlite_stmt_release;
	stmt->fn_bind = vtm_sqlite_stmt_bind;
	stmt->fn_execute = vtm_sqlite_stmt_execute;
	stmt->fn_query = vtm_sqlite_stmt_query;

	return VTM_OK;

err_buf:
	vtm_buf_release(&query_buf);
	vtm_list_free(stmt_data->param_names);

err_data:
	free(stmt_data);

	return rc;
}

void vtm_sqlite_stmt_release(struct vtm_sql_stmt *stmt)
{
	struct vtm_sqlite_stmt_data *stmt_data;

	stmt_data = stmt->stmt_data;

	sqlite3_finalize(stmt_data->stmt);
	vtm_list_free(stmt_data->param_names);
	free(stmt_data);
}

int vtm_sqlite_stmt_bind(struct vtm_sql_stmt *stmt, vtm_dataset *bind)
{
	int rc;
	struct vtm_sqlite_stmt_data *stmt_data;
	struct vtm_variant *var;
	const char *name;
	size_t i, count;

	stmt_data = stmt->stmt_data;

	rc = sqlite3_clear_bindings(stmt_data->stmt);
	if (rc != SQLITE_OK)
		return vtm_err_set(VTM_E_SQL_UNKNOWN);

	count = vtm_list_size(stmt_data->param_names);
	for (i=0; i < count; i++) {
		name = vtm_list_get_pointer(stmt_data->param_names, i);
		var = vtm_dataset_get_variant(bind, name);
		rc = vtm_sqlite_stmt_bind_variant(stmt_data->stmt, (int) i+1, var);
		if (rc != VTM_OK)
			return rc;
	}

	return VTM_OK;
}

int vtm_sqlite_stmt_execute(struct vtm_sql_stmt *stmt)
{
	int rc;
	struct vtm_sqlite_stmt_data *stmt_data;

	stmt_data = stmt->stmt_data;

	rc = sqlite3_reset(stmt_data->stmt);
	if (rc != SQLITE_OK)
		return vtm_sqlite_error(rc, stmt_data->con);

	rc = sqlite3_step(stmt_data->stmt);
	switch (rc) {
		case SQLITE_DONE:
			rc = VTM_OK;
			break;

		case SQLITE_ROW:
			rc = vtm_err_set(VTM_E_SQL_EXEC_RESULT);
			break;

		default:
			rc = vtm_err_set(VTM_E_SQL_UNKNOWN);
			break;
	}

	return rc;
}

int vtm_sqlite_stmt_query(struct vtm_sql_stmt *stmt, struct vtm_sql_result *result)
{
	int rc;
	struct vtm_sqlite_stmt_data *stmt_data;

	stmt_data = stmt->stmt_data;

	rc = sqlite3_reset(stmt_data->stmt);
	if (rc != SQLITE_OK)
		return vtm_sqlite_error(rc, stmt_data->con);

	rc = sqlite3_step(stmt_data->stmt);
	if (rc != SQLITE_DONE && rc != SQLITE_ROW)
		return VTM_E_SQL_UNKNOWN;
	stmt_data->last_step = rc;

	if (sqlite3_column_count(stmt_data->stmt) < 1)
		return VTM_E_SQL_EXEC_RESULT;

	vtm_sql_result_init(result);
	rc = vtm_sqlite_stmt_save_result_columns(stmt_data, result);
	if (rc != VTM_OK) {
		vtm_sql_result_release(result);
		return rc;
	}

	result->fn_fetch_row = vtm_sqlite_stmt_result_fetch_row;
	result->fn_fetch_all = vtm_sqlite_stmt_result_fetch_all;
	result->fn_finish = vtm_sqlite_stmt_result_finish;
	result->fn_release = vtm_sql_result_release_default;

	result->res_data = stmt_data;
	result->res_owner = stmt;

	stmt->state = VTM_SQL_STMT_STATE_OPEN_RESULT;

	return VTM_OK;
}

static int vtm_sqlite_stmt_bind_variant(sqlite3_stmt *stmt, int index, struct vtm_variant *var)
{
	int rc;
	const void *blob;

	if (!var) {
		sqlite3_bind_null(stmt, index);
		return VTM_OK;
	}

	switch (var->type) {
		case VTM_ELEM_NULL:
			rc = sqlite3_bind_null(stmt, index);
			break;

		case VTM_ELEM_INT8:
		case VTM_ELEM_UINT8:
		case VTM_ELEM_BOOL:
		case VTM_ELEM_CHAR:
		case VTM_ELEM_SCHAR:
		case VTM_ELEM_UCHAR:
		case VTM_ELEM_INT16:
		case VTM_ELEM_UINT16:
		case VTM_ELEM_SHORT:
		case VTM_ELEM_USHORT:
			rc = sqlite3_bind_int(stmt, index, vtm_variant_as_int(var));
			break;

		case VTM_ELEM_INT32:
		case VTM_ELEM_UINT32:
		case VTM_ELEM_INT:
		case VTM_ELEM_UINT:
		case VTM_ELEM_INT64:
		case VTM_ELEM_UINT64:
		case VTM_ELEM_LONG:
		case VTM_ELEM_ULONG:
			rc = sqlite3_bind_int64(stmt, index, vtm_variant_as_int64(var));
			break;

		case VTM_ELEM_FLOAT:
		case VTM_ELEM_DOUBLE:
			rc = sqlite3_bind_double(stmt, index, vtm_variant_as_double(var));
			break;

		case VTM_ELEM_STRING:
			rc = sqlite3_bind_text(stmt, index, vtm_variant_as_str(var),
				-1, SQLITE_TRANSIENT);
			break;

		case VTM_ELEM_BLOB:
			blob = vtm_variant_as_blob(var);
			rc = sqlite3_bind_blob(stmt, index, blob, vtm_blob_size(blob),
				SQLITE_TRANSIENT);
			break;

		default:
			return VTM_E_NOT_SUPPORTED;
	}

	if (rc != SQLITE_OK)
		return VTM_ERROR;

	return VTM_OK;
}

static int vtm_sqlite_stmt_result_fetch_row(struct vtm_sql_result *res, vtm_dataset *row)
{
	int rc;
	struct vtm_sqlite_stmt_data *stmt_data;

	stmt_data = res->res_data;
	switch (stmt_data->last_step) {
		case SQLITE_DONE:
			return VTM_E_SQL_RESULT_COMPLETE;

		case SQLITE_ROW:
			break;

		default:
			return vtm_err_set(VTM_E_SQL_UNKNOWN);
	}

	rc = vtm_sqlite_stmt_save_row_values(stmt_data, res, row);
	if (rc != VTM_OK)
		return rc;

	stmt_data->last_step = sqlite3_step(stmt_data->stmt);

	return VTM_OK;
}

static int vtm_sqlite_stmt_result_fetch_all(struct vtm_sql_result *res)
{
	int rc;
	struct vtm_sqlite_stmt_data *stmt_data;
	size_t allocated_rows, allocated_bytes, old;
	void *grown;

	stmt_data = res->res_data;

	allocated_rows = 16;
	allocated_bytes = allocated_rows * sizeof(struct vtm_dataset);
	res->rows = malloc(allocated_bytes);
	if (!res->rows) {
		vtm_err_oom();
		return vtm_err_get_code();
	}

	while (stmt_data->last_step == SQLITE_ROW) {
		if (res->row_count == allocated_rows) {
			old = allocated_bytes;
			allocated_bytes *= 2;
			if (old > allocated_bytes)
				return VTM_E_OVERFLOW;

			grown = realloc(res->rows, allocated_bytes);
			if(!grown) {
				vtm_err_oom();
				return vtm_err_get_code();
			}
			res->rows = grown;

			old = allocated_rows;
			allocated_rows *= 2;
			if (old > allocated_rows)
				return VTM_E_OVERFLOW;
		}

		vtm_dataset_init(&res->rows[res->row_count], VTM_DS_HINT_DEFAULT);
		rc = vtm_sqlite_stmt_save_row_values(stmt_data, res,
		                                     &res->rows[res->row_count]);
		res->row_count++;
		if (rc != VTM_OK)
			return rc;

		stmt_data->last_step = sqlite3_step(stmt_data->stmt);
	}

	if (stmt_data->last_step != SQLITE_DONE)
		return vtm_sqlite_error(stmt_data->last_step, stmt_data->con);

	return VTM_OK;
}

static void vtm_sqlite_stmt_result_finish(struct vtm_sql_result *res)
{
	struct vtm_sql_stmt *stmt;
	struct vtm_sqlite_stmt_data *stmt_data;

	stmt = res->res_owner;
	stmt->state = VTM_SQL_STMT_STATE_READY;

	stmt_data = stmt->stmt_data;
	free(stmt_data->column_types);
	stmt_data->col_count = 0;
	stmt_data->column_types = NULL;
}

static int vtm_sqlite_stmt_save_result_columns(struct vtm_sqlite_stmt_data *stmt_data, struct vtm_sql_result *res)
{
	int i;
	sqlite3_stmt *stmt;

	stmt = stmt_data->stmt;

	stmt_data->col_count = sqlite3_column_count(stmt);
	stmt_data->column_types = calloc(stmt_data->col_count, sizeof(int));
	if (!stmt_data->column_types) {
		vtm_err_oom();
		return vtm_err_get_code();
	}

	res->columns = calloc(stmt_data->col_count , sizeof(char*));
	if (!res->columns) {
		vtm_err_oom();
		return vtm_err_get_code();
	}
	res->col_count = stmt_data->col_count ;

	for (i=0; i < stmt_data->col_count; i++) {
		stmt_data->column_types[i] = sqlite3_column_type(stmt, i);
		res->columns[i] = vtm_str_copy(sqlite3_column_name(stmt, i));
		if (!res->columns[i])
			return vtm_err_get_code();
	}

	return VTM_OK;
}

static int vtm_sqlite_stmt_save_row_values(struct vtm_sqlite_stmt_data *stmt_data, struct vtm_sql_result *res, vtm_dataset *row)
{
	int i;
	sqlite3_stmt *stmt;
	size_t len;
	void *blob;

	stmt = stmt_data->stmt;

	for (i=0; i < stmt_data->col_count; i++) {
		switch (stmt_data->column_types[i]) {
			case SQLITE_NULL:
				break;

			case SQLITE_INTEGER:
				vtm_dataset_set_int64(row, res->columns[i], sqlite3_column_int64(stmt, i));
				break;

			case SQLITE_FLOAT:
				vtm_dataset_set_double(row, res->columns[i], sqlite3_column_double(stmt, i));
				break;

			case SQLITE_TEXT:
				vtm_dataset_set_string(row, res->columns[i], (const char*) sqlite3_column_text(stmt, i));
				break;

			case SQLITE_BLOB:
				len = sqlite3_column_bytes(stmt, i);
				blob = vtm_blob_new(len);
				if (!blob)
					return vtm_err_get_code();

				memcpy(blob, sqlite3_column_blob(stmt, i), len);
				vtm_dataset_set_blob(row, res->columns[i], blob);
				break;
		}
	}

	return VTM_OK;
}
