/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_SQL_SQL_UTIL_INTL_H_
#define VTM_SQL_SQL_UTIL_INTL_H_

#include <vtm/core/buffer.h>
#include <vtm/core/list.h>

#ifdef __cplusplus
extern "C" {
#endif

int vtm_sql_extract_query_params(struct vtm_buf *buf, const char *query, vtm_list *names);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_SQL_UTIL_INTL_H_ */
