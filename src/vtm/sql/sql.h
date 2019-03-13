/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file sql.h
 *
 * @brief SQL module basics
 */

#ifndef VTM_SQL_SQL_H_
#define VTM_SQL_SQL_H_

#include <vtm/core/api.h>
#include <vtm/sql/sql_connection.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vtm_sql_module
{
	vtm_sql_con* (*con_new)(vtm_dataset *param);
	void (*module_thread_end)(void);
	void (*module_end)(void);
};

/**
 * Tell SQL implementation that all resources for the calling
 * thread should be released.
 *
 * This should be the last call from the thread to interact
 * with the given SQL implementation.
 *
 * @param mod the initialized SQL module
 */
VTM_API void vtm_module_sql_thread_end(struct vtm_sql_module *mod);

/**
 * Tell SQL implementation that all resources should be released.
 *
 * This should be the last call to interact with the SQL implementation.
 *
 * @param mod the initialized SQL module
 */
VTM_API void vtm_module_sql_end(struct vtm_sql_module *mod);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SQL_SQL_H_ */
