/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file socket_addr.h
 *
 * @brief Socket address
 */

#ifndef VTM_NET_SOCKET_ADDR_H_
#define VTM_NET_SOCKET_ADDR_H_

#include <vtm/core/api.h>
#include <vtm/net/socket_spec.h>

#if defined(VTM_SYS_UNIX) || defined(VTM_SYS_WINDOWS)
	#include <vtm/sys/base/net/socket_saddr.h>
#else
	#error no socket implementation
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Localhost constant */
#define VTM_SOCK_ADDR_LOCALHOST         "localhost"

/** IPv4 loopback address */
#define VTM_SOCK_ADDR_IPV4_LOOPBACK     "127.0.0.1"
/** IPv4 any address placeholder */
#define VTM_SOCK_ADDR_IPV4_ANY          "0.0.0.0"

/** IPv6 loopback address */
#define VTM_SOCK_ADDR_IPV6_LOOPBACK     "::1"
/** IPv6 any address placeholder */
#define VTM_SOCK_ADDR_IPV6_ANY          "::"

/** Minimum buffer size to hold string representation of an IP address */
#define VTM_SOCK_ADDR_BUF_LEN           46

struct vtm_socket_addr
{
	enum vtm_socket_family   family;  /**< the socket family */
	const char               *host;   /**< hostname or ip address */
	unsigned int             port;    /**< port number, range 0-65535 */
};

/**
 * Converts a readable address in a platform/os specific representation.
 *
 * @param[out] saddr the converted address
 * @param from the address that should be converted
 * @return VTM_OK if the conversion was successful
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_os_addr_build(struct vtm_socket_saddr *saddr, struct vtm_socket_addr *from);

/**
 * Convertes a platform specific address representation back to a readable
 * format.
 *
 * @param saddr the address that should converted
 * @param[out] fam  the socket family
 * @param[out] host_buf the buffer where the ip address is stored
 * @param len the length in bytes of the host buffer
 * @param[out] port the port of the address
 * @return VTM_OK if the conversion was successful
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_os_addr_convert(struct vtm_socket_saddr *saddr, enum vtm_socket_family *fam,
	char *host_buf, size_t len, unsigned int *port);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_SOCKET_ADDR_H_ */
