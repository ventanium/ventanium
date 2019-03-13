/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file sql_result.h
 *
 * @brief Result set
 */

#ifndef VTM_SQL_SQL_RESULT_H_
#define VTM_SQL_SQL_RESULT_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/core/dataset.h>

#ifdef __cplusplus
extern "C" {
#endif

enum vtm_sql_result_state
{
	VTM_SQL_RES_STATE_UNINIT,
	VTM_SQL_RES_STATE_READY,
	VTM_SQL_RES_STATE_STEPPING,
	VTM_SQL_RES_STATE_FINISHED
};

struct vtm_sql_result
{
	/* public */
	char **columns;      /**< pointer to array of column names */
	size_t col_count;    /**< number of columns */

	vtm_dataset *rows;   /**< pointer to array of rows */
	size_t row_count;    /**< number of rows */

	/* private */
	enum vtm_sql_result_state state;
	void *res_data;
	void *res_owner;

	int  (*fn_fetch_row)(struct vtm_sql_result *res, vtm_dataset *row);
	int  (*fn_fetch_all)(struct vtm_sql_result *res);
	void (*fn_finish)(struct vtm_sql_result *res);
	void (*fn_release)(struct vtm_sql_result *res);
	void (*fn_release_owner)(void *owner);
};

/**
 * Releases the result set.
 *
 * @param res the result set that should be released
 */
VTM_API void vtm_sql_result_release(struct vtm_sql_result *res);

/**
 * Fetches the next row from the result set.
 *
 * @param res the target result set
 * @param[out] row the fetched row
 * @return VTM_OK if row was sucessfully retrieved
 * @return VTM_E_IO_EOF if all rows were retrieved
 * @return VTM_E_INVALID_STATE if the result set is in the wrong state, for
 *         example if all rows have alread been fetched
 * @return VTM_E_NOT_SUPPORTED if the operation is not supported in the
 *         underlaying sql implementation
 * @return VTM_ERROR if an unknown error occured
 */
VTM_API int vtm_sql_result_fetch_row(struct vtm_sql_result *res, vtm_dataset *row);

/**
 * Fetches and buffers the complete result set.
 *
 * @param res the target result set
 * @return VTM_OK if operation succeed
 * @return VTM_E_INVALID_STATE if the result set is in the wrong state, for
 *         example if single row fetching has already been used.
 * @return VTM_E_NOT_SUPPORTED if the operation is not supported in the
 *         underlaying sql implementation
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_sql_result_fetch_all(struct vtm_sql_result *res);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_SQL_RESULT_H_ */
