/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "sqlite.h"

#include <sqlite3.h>
#include <vtm/core/error.h>
#include <vtm/sql/sqlite/sqlite_connection_intl.h>

/* forward declaration */
static void vtm_module_sqlite_end(void);

int vtm_module_sqlite_init(struct vtm_sql_module *mod)
{
	if (sqlite3_initialize() != SQLITE_OK)
		return VTM_ERROR;
	
	mod->con_new = vtm_sqlite_con_new;
	mod->module_thread_end = NULL;
	mod->module_end = vtm_module_sqlite_end;
	
	return VTM_OK;
}

static void vtm_module_sqlite_end(void)
{
	sqlite3_shutdown();
}
