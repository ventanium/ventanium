/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "sql_result.h"

#include <string.h> /* memset() */
#include <vtm/core/error.h>

/* forward declaration */
static void vtm_sql_result_finish(struct vtm_sql_result *res);

void vtm_sql_result_init(struct vtm_sql_result *res)
{
	memset(res, 0, sizeof(*res));
	res->state = VTM_SQL_RES_STATE_READY;
}

void vtm_sql_result_release(struct vtm_sql_result *res)
{
	if (!res)
		return;

	if (res->fn_release_owner && res->res_owner)
		res->fn_release_owner(res->res_owner);

	if (res->fn_release)
		res->fn_release(res);
}

void vtm_sql_result_release_default(struct vtm_sql_result *res)
{
	size_t i;

	if (res->columns) {
		for (i=0; i < res->col_count; i++)
			free(res->columns[i]);

		free(res->columns);
	}

	if (res->rows) {
		for (i=0; i < res->row_count; i++)
			vtm_dataset_release(&res->rows[i]);
			
		free(res->rows);
	}
}

int vtm_sql_result_fetch_row(struct vtm_sql_result *res, vtm_dataset *row)
{
	int rc;
	
	if (!res)
		return VTM_E_INVALID_ARG;

	if (res->state == VTM_SQL_RES_STATE_READY)
		res->state = VTM_SQL_RES_STATE_STEPPING;
	else if (res->state != VTM_SQL_RES_STATE_STEPPING)
		return VTM_E_INVALID_STATE;

	if (!res->fn_fetch_row)
		return VTM_E_NOT_SUPPORTED;

	rc = res->fn_fetch_row(res, row);
	if (rc != VTM_OK)
		vtm_sql_result_finish(res);

	return rc;
}

int vtm_sql_result_fetch_all(struct vtm_sql_result *res)
{
	int rc;
	
	if (!res)
		return VTM_E_INVALID_ARG;

	if (res->state != VTM_SQL_RES_STATE_READY)
		return VTM_E_INVALID_STATE;

	if (!res->fn_fetch_all)
		return VTM_E_NOT_SUPPORTED;

	rc = res->fn_fetch_all(res);
	vtm_sql_result_finish(res);

	return rc;
}

static void vtm_sql_result_finish(struct vtm_sql_result *res)
{
	res->state = VTM_SQL_RES_STATE_FINISHED;

	if (res->fn_finish)
		res->fn_finish(res);
}
