/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_SQL_MYSQL_MYSQL_BIND_BUFFER_INTL_H_
#define VTM_SQL_MYSQL_MYSQL_BIND_BUFFER_INTL_H_

#include <mysql.h>
#include <vtm/core/types.h>
#include <vtm/core/dataset.h>
#include <vtm/core/variant.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vtm_mysql_bind_buf
{
	const char *name;
	enum enum_field_types type;
	unsigned long content_length;
	my_bool content_null;

	void *data;
	union vtm_elem static_data;
};

int vtm_mysql_bind_buf_init_from_field(struct vtm_mysql_bind_buf *buf, MYSQL_FIELD *field);
int vtm_mysql_bind_buf_init_from_variant(struct vtm_mysql_bind_buf *buf, struct vtm_variant *var);

void vtm_mysql_bind_buf_release(struct vtm_mysql_bind_buf *buf);

int vtm_mysql_bind_buf_read(struct vtm_mysql_bind_buf *buf, vtm_dataset *ds);
void vtm_mysql_bind_buf_couple(struct vtm_mysql_bind_buf *buf, MYSQL_BIND *my_bind);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_MYSQL_MYSQL_BIND_BUFFER_INTL_H_ */
