/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file process.h
 *
 * @brief Process helper functions
 */

#ifndef VTM_UTIL_PROCESS_H_
#define VTM_UTIL_PROCESS_H_

#include <vtm/core/api.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gets the id of the current process.
 *
 * @return the unique id of the current process
 */
VTM_API unsigned long vtm_process_get_current_id();

#ifdef __cplusplus
}
#endif

#endif /* VTM_UTIL_PROCESS_H_ */
