/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_SQL_MYSQL_MYSQL_ERROR_INTL_H_
#define VTM_SQL_MYSQL_MYSQL_ERROR_INTL_H_

#include <mysql.h>

#ifdef __cplusplus
extern "C" {
#endif

int vtm_mysql_error(MYSQL *my);
int vtm_mysql_error_stmt(MYSQL_STMT *stmt);
int vtm_mysql_convert_errno(unsigned int mysql_err);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_MYSQL_MYSQL_ERROR_INTL_H_ */
