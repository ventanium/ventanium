/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file sqlite.h
 *
 * @brief SQLite3 Connector
 */

#ifndef VTM_SQL_SQLITE_SQLITE_H_
#define VTM_SQL_SQLITE_SQLITE_H_

#include <vtm/core/api.h>
#include <vtm/sql/sql.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the SQLite 3 connector.
 *
 * After the call succeed, you can use the module to open
 * SQLite3 database connections.
 *
 * @param mod the module variable that should be initialized
 * @return VTM_OK if the initialization was successful
 * @return VTM_ERROR if module could not be initialized
 */
VTM_API int vtm_module_sqlite_init(struct vtm_sql_module *mod);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_SQLITE_SQLITE_H_ */
