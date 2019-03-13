/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "mysql.h"

#include <mysql.h>

#include <vtm/core/error.h>
#include <vtm/sql/mysql/mysql_connection_intl.h>

int vtm_module_mysql_init(struct vtm_sql_module *mod)
{
	mod->con_new = vtm_mysql_con_new;
	mod->module_thread_end = mysql_thread_end;
	mod->module_end = mysql_library_end;
	
	return VTM_OK;
}
