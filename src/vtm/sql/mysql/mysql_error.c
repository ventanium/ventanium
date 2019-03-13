/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "mysql_error_intl.h"

#include <mysqld_error.h>
#include <errmsg.h>
#include <vtm/core/error.h>
#include <vtm/sql/sql_error.h>

int vtm_mysql_error(MYSQL *my)
{
	int rc;
	
	rc = vtm_mysql_convert_errno(mysql_errno(my));
	vtm_err_setf(rc, "%d: %s", mysql_errno(my), mysql_error(my));
	
	return rc;
}

int vtm_mysql_error_stmt(MYSQL_STMT *stmt)
{
	int rc;
	
	rc = vtm_mysql_convert_errno(mysql_stmt_errno(stmt));
	vtm_err_setf(rc, "%d: %s", mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
	
	return rc;
}

int vtm_mysql_convert_errno(unsigned int mysql_err)
{
	switch (mysql_err) {
		case ER_DUP_ENTRY:
			return VTM_E_SQL_DUPLICATE_ENTRY;
		
		case CR_SERVER_GONE_ERROR:
		case CR_SERVER_LOST:
			return VTM_E_SQL_CONNECTION_LOST;

		default:
			break;
	}

	return VTM_E_SQL_UNKNOWN;
}
