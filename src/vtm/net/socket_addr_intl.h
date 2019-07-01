/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#ifndef VTM_NET_SOCKET_ADDR_INTL_H_
#define VTM_NET_SOCKET_ADDR_INTL_H_

#include <vtm/net/socket.h>
#include <vtm/net/socket_addr.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VTM_SOCK_ADDR_TYPE_INVALID          0

#define VTM_SOCK_ADDR_TYPE_IPV4_LOOPBACK    1
#define VTM_SOCK_ADDR_TYPE_IPV4_SPEC        2
#define VTM_SOCK_ADDR_TYPE_IPV4_ANY         3

#define VTM_SOCK_ADDR_TYPE_IPV6_LOOPBACK    4
#define VTM_SOCK_ADDR_TYPE_IPV6_SPEC        5
#define VTM_SOCK_ADDR_TYPE_IPV6_ANY         6

struct vtm_socket_addr_info
{
	enum vtm_socket_family sock_family;
	int addr_type;
};

int vtm_socket_addr_get_info(const char *addr, struct vtm_socket_addr_info *info);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_SOCKET_ADDR_INTL_H_ */
