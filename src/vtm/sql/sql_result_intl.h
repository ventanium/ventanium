/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_SQL_SQL_RESULT_INTL_H_
#define VTM_SQL_SQL_RESULT_INTL_H_

#include <vtm/sql/sql_result.h>

#ifdef __cplusplus
extern "C" {
#endif

void vtm_sql_result_init(struct vtm_sql_result *res);
void vtm_sql_result_release_default(struct vtm_sql_result *res);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_SQL_RESULT_INTL_H_ */
