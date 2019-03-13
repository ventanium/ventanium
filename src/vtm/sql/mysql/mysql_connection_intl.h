/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_SQL_MYSQL_MYSQL_CONNECTION_INTL_H_
#define VTM_SQL_MYSQL_MYSQL_CONNECTION_INTL_H_

#include <vtm/core/dataset.h>
#include <vtm/sql/sql_connection.h>

#ifdef __cplusplus
extern "C" {
#endif

vtm_sql_con* vtm_mysql_con_new(vtm_dataset *param);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_MYSQL_MYSQL_CONNECTION_INTL_H_ */
