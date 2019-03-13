/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_SQL_MYSQL_MYSQL_BIND_INTL_H_
#define VTM_SQL_MYSQL_MYSQL_BIND_INTL_H_

#include <mysql.h>
#include <vtm/core/dataset.h>
#include <vtm/core/types.h>
#include <vtm/sql/mysql/mysql_bind_buffer_intl.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vtm_mysql_bind
{
	MYSQL_BIND *bindings;
	struct vtm_mysql_bind_buf *buffers;
	size_t count;
};

int vtm_mysql_bind_init(struct vtm_mysql_bind *bind, size_t count);
void vtm_mysql_bind_release(struct vtm_mysql_bind *bind);

int vtm_mysql_bind_write(struct vtm_mysql_bind *bind, vtm_dataset *in, vtm_list *order);
int vtm_mysql_bind_read(struct vtm_mysql_bind *bind, vtm_dataset *out);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_MYSQL_MYSQL_BIND_INTL_H_ */
