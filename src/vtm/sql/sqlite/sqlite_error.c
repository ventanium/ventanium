/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "sqlite_error_intl.h"

#include <vtm/core/error.h>
#include <vtm/sql/sql_error.h>

int vtm_sqlite_error_intl(int code, sqlite3 *con, const char *file, unsigned long line)
{
	return vtm_err_setf_intl(VTM_E_SQL_UNKNOWN, NULL, file, line, sqlite3_errmsg(con));
}
