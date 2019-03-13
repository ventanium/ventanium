/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "mysql_bind_intl.h"

#include <stdlib.h> /* calloc() */
#include <vtm/core/error.h>
#include <vtm/sql/sql_error.h>
#include <vtm/sql/mysql/mysql_error_intl.h>

int vtm_mysql_bind_init(struct vtm_mysql_bind *bind, size_t count)
{
	bind->count = count;

	bind->bindings = calloc(count, sizeof(MYSQL_BIND));
	if (!bind->bindings) {
		vtm_err_oom();
		return vtm_err_get_code();
	}
	
	bind->buffers = calloc(count, sizeof(struct vtm_mysql_bind_buf));
	if (!bind->bindings) {
		vtm_err_oom();
		free(bind->bindings);
		return vtm_err_get_code();
	}

	return VTM_OK;
}

void vtm_mysql_bind_release(struct vtm_mysql_bind *bind)
{
	size_t i;

	if (!bind)
		return;
	
	for (i=0; i < bind->count; i++) {
		vtm_mysql_bind_buf_release(&bind->buffers[i]);
	}
	
	free(bind->bindings);
	free(bind->buffers);
}

int vtm_mysql_bind_write(struct vtm_mysql_bind *bind, vtm_dataset *in, vtm_list *order)
{
	size_t i;
	const char *name;
	struct vtm_variant *var;

	if (bind->count != vtm_list_size(order))
		return VTM_E_SQL_PARAMCOUNT;
	
	for (i=0; i < bind->count; i++) {
		name = vtm_list_get_pointer(order, i);
		var = vtm_dataset_get_variant(in, name);

		vtm_mysql_bind_buf_init_from_variant(&bind->buffers[i], var);
		vtm_mysql_bind_buf_couple(&bind->buffers[i], &bind->bindings[i]);
	}

	return VTM_OK;
}

int vtm_mysql_bind_read(struct vtm_mysql_bind *bind, vtm_dataset *out)
{
	int rc;
	size_t i;

	for (i=0; i < bind->count; i++) {
		rc = vtm_mysql_bind_buf_read(&bind->buffers[i], out);
		if (rc != VTM_OK)
			return rc;
	}

	return VTM_OK;
}
