/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_SYS_BASE_NET_SOCKET_TYPES_INTL_H_
#define VTM_SYS_BASE_NET_SOCKET_TYPES_INTL_H_

#ifdef VTM_SYS_UNIX

	#include <vtm/core/types.h>

	typedef int vtm_sys_socket_t;
	typedef ssize_t vtm_sys_sockrc_t;

	#define VTM_CLOSESOCKET(FD)        close(FD)
	#define VTM_SOCK_INVALID(FD)       (FD < 0)

#elif VTM_SYS_WINDOWS

	#include <windows.h>
	typedef SOCKET vtm_sys_socket_t;
	typedef int vtm_sys_sockrc_t;

	#define VTM_CLOSESOCKET(FD)        closesocket(FD)
	#define VTM_SOCK_INVALID(FD)       (FD == INVALID_SOCKET)

#endif

#define VTM_SOCK_ERR(rc)              (rc != 0)

#endif /* VTM_SYS_BASE_NET_SOCKET_TYPES_INTL_H_ */
