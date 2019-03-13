/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file mysql.h
 *
 * @brief MySQL / MariaDB Connector
 */

#ifndef VTM_SQL_MYSQL_MYSQL_H_
#define VTM_SQL_MYSQL_MYSQL_H_

#include <vtm/core/api.h>
#include <vtm/sql/sql.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the MySQL connector.
 *
 * After the call succeed, you can use the module to open
 * MySQL or MariaDB database connections.
 *
 * @param mod the module variable that should be initialized
 * @return VTM_OK if the initialization was successful
 * @return VTM_ERROR if module could not be initialized
 */
VTM_API int vtm_module_mysql_init(struct vtm_sql_module *mod);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_MYSQL_MYSQL_H_ */
