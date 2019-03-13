/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file system.h
 *
 * @brief Platform and OS dependent settings
 */

#ifndef VTM_CORE_SYSTEM_H_
#define VTM_CORE_SYSTEM_H_

#include <vtm/core/api.h>

#ifdef __cplusplus
extern "C" {
#endif

enum vtm_byteorder
{
	VTM_BYTEORDER_BE,  /**< Big-endian */
	VTM_BYTEORDER_LE   /**< Little-endian */
};

/**
 * Gets the host byte order.
 *
 * @return byte order of current platform
 */
VTM_API enum vtm_byteorder vtm_sys_get_byteorder(void);

#ifdef __cplusplus
}
#endif

#endif /* VTM_CORE_SYSTEM_H_ */
