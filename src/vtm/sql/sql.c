/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "sql.h"

void vtm_module_sql_thread_end(struct vtm_sql_module *mod)
{
	if (mod->module_thread_end)
		mod->module_thread_end();
}

void vtm_module_sql_end(struct vtm_sql_module *mod)
{
	if (mod->module_end)
		mod->module_end();
}
