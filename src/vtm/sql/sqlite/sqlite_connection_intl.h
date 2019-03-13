/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_SQL_SQLITE_SQLITE_CONNECTION_INTL_H_
#define VTM_SQL_SQLITE_SQLITE_CONNECTION_INTL_H_

#include <vtm/sql/sql.h>

#ifdef __cplusplus
extern "C" {
#endif

vtm_sql_con* vtm_sqlite_con_new(vtm_dataset *param);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_SQLITE_SQLITE_CONNECTION_INTL_H_ */
