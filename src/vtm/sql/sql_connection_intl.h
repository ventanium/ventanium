/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_SQL_SQL_CONNECTION_INTL_H_
#define VTM_SQL_SQL_CONNECTION_INTL_H_

#include <vtm/core/dataset.h>
#include <vtm/sql/sql_error.h>

#ifdef __cplusplus
extern "C" {
#endif

enum vtm_sql_con_state
{
	VTM_SQL_CON_STATE_READY,
	VTM_SQL_CON_STATE_OPEN_RESULT
};

struct vtm_sql_con_vt
{
	void *con_data;
	enum vtm_sql_con_state state;

	int (*fn_set_lock_wait_timeout)(struct vtm_sql_con_vt *con, unsigned long millis);
	int (*fn_set_auto_commit)(struct vtm_sql_con_vt *con, bool commit);
	int (*fn_commit)(struct vtm_sql_con_vt *con);
	int (*fn_rollback)(struct vtm_sql_con_vt *con);
	int (*fn_prepare)(struct vtm_sql_con_vt *con, const char *query, struct vtm_sql_stmt *stmt);
	int (*fn_execute)(struct vtm_sql_con_vt *con, const char *query);
	int (*fn_query)(struct vtm_sql_con_vt *con, const char *query, struct vtm_sql_result *result);
	int (*fn_free)(struct vtm_sql_con_vt *con);
};

void vtm_sql_stmt_free(void *stmt);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_SQL_CONNECTION_INTL_H_ */
