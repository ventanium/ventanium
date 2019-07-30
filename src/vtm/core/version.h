/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file version.h
 *
 * @brief Version info
 */

#ifndef VTM_CORE_VERSION_H_
#define VTM_CORE_VERSION_H_

#include <vtm/core/macros.h>

#define VTM_VERSION_MAJOR       0
#define VTM_VERSION_MINOR       1
#define VTM_VERSION_PATCH       0

#define VTM_BUILD_NAME          "ventanium"

#define VTM_BUILD_VERSION             \
	VTM_BUILD_NAME "/"                \
	VTM_TO_STR(VTM_VERSION_MAJOR) "." \
	VTM_TO_STR(VTM_VERSION_MINOR) "." \
	VTM_TO_STR(VTM_VERSION_PATCH)

#endif /* VTM_CORE_VERSION_H_ */
