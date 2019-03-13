/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file path.h
 *
 * @brief Helper functions dealing with relative and absolute paths
 */

#ifndef VTM_FS_PATH_H_
#define VTM_FS_PATH_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the full absolute path for the given input path.
 *
 * @param path the path to convert
 * @param[out] out_resolved an allocated buffer containing the real path.
 *             This buffer must be freed by the caller.
 * @return VTM_OK if the real path could be determined successfully
 * @return VTM_E_IO_UNKNOWN if an error occured
 */
VTM_API int vtm_path_get_real(const char *path, char **out_resolved);

#ifdef __cplusplus
}
#endif

#endif /* VTM_FS_PATH_H_ */
