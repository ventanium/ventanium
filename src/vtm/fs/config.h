/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file config.h
 *
 * @brief Support for loading a simple configuration file
 */

#ifndef VTM_FS_CONFIG_H_
#define VTM_FS_CONFIG_H_

#include <vtm/core/api.h>
#include <vtm/core/dataset.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Trys to open and parse the given config file
 *
 * @param file the config file including path which should be opened
 * @return the loaded configuration stored in a dataset
 * @return NULL if the file could not be read successfully
 */
VTM_API vtm_dataset* vtm_config_file_read(const char *file);

#ifdef __cplusplus
}
#endif

#endif /* VTM_FS_CONFIG_H_ */
