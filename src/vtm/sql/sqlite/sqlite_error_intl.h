/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_SQL_SQLITE_SQLITE_ERROR_INTL_H_
#define VTM_SQL_SQLITE_SQLITE_ERROR_INTL_H_

#include <sqlite3.h>

#ifdef __cplusplus
extern "C" {
#endif

#define vtm_sqlite_error(CODE, CON)   vtm_sqlite_error_intl((CODE), (CON), __FILE__, __LINE__);

int vtm_sqlite_error_intl(int code, sqlite3 *con, const char *file, unsigned long line);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_SQLITE_SQLITE_ERROR_INTL_H_ */
