/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_SYS_BASE_NET_SOCKET_TYPES_H_
#define VTM_SYS_BASE_NET_SOCKET_TYPES_H_

#include <vtm/core/api.h>

#ifdef VTM_SYS_UNIX

#include <sys/socket.h>
typedef socklen_t vtm_sys_socklen_t;

#elif VTM_SYS_WINDOWS

typedef int vtm_sys_socklen_t;

#endif

#endif /* VTM_SYS_BASE_NET_SOCKET_TYPES_H_ */
