/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_SYS_BASE_NET_SOCKET_SADDR_H_
#define VTM_SYS_BASE_NET_SOCKET_SADDR_H_

#include <vtm/core/api.h>

#ifdef VTM_SYS_UNIX

#include <netinet/in.h>
#include <sys/socket.h>

#elif VTM_SYS_WINDOWS

/* ws2tcpip.h MUST be included before windows.h */
#include <ws2tcpip.h>
#include <windows.h>

#endif

#include <vtm/sys/base/net/socket_types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vtm_socket_saddr
{
	union {
		struct sockaddr        sa;
		struct sockaddr_in     in4;
		struct sockaddr_in6    in6;
	}                          addr;
	vtm_sys_socklen_t          len;
};

#ifdef __cplusplus
}
#endif

#endif /* VTM_SYS_BASE_NET_SOCKET_SADDR_H_ */
