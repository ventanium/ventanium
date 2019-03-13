/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file network.h
 *
 * @brief Network module basics
 */

#ifndef VTM_NET_NETWORK_H_
#define VTM_NET_NETWORK_H_

#include <vtm/core/api.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the network module.
 *
 * This function must be called before you can use any other network function.
 * On Windows this initializes the Windows Socket Api (WSA).
 *
 * @return VTM_OK if initialization was successful or is not needed on current
 *                platform
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_module_network_init(void);

/**
 * Deinitializes the network module.
 *
 * All allocated resources are released.
 *
 * On Windows this will shut down the WSA subsystem.
 */
VTM_API void vtm_module_network_end(void);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_NETWORK_H_ */
