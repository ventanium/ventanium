/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file socket_spec.h
 *
 * @brief Socket specification
 */

#ifndef VTM_NET_SOCKET_SPEC_H_
#define VTM_NET_SOCKET_SPEC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* protocols */
#define VTM_SOCK_TYPE_UNSPEC        0     /**< unspecified socket type */
#define VTM_SOCK_TYPE_STREAM        1     /**< stream type, for example TCP */
#define VTM_SOCK_TYPE_DGRAM         2     /**< datagram type, for example UDP */

/* convience aliases */
#define VTM_SOCK_TYPE_TCP           VTM_SOCK_TYPE_STREAM  /**< TCP socket */
#define VTM_SOCK_TYPE_UDP           VTM_SOCK_TYPE_DGRAM   /**< UDP socket */

enum vtm_socket_family
{
	VTM_SOCK_FAM_IN4,   /**< IPv4 */
	VTM_SOCK_FAM_IN6    /**< IPv6 */
};

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_SOCKET_SPEC_H_ */
