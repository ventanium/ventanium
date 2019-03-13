/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file socket_shared.h
 *
 * @brief Shared socket structures
 */

#ifndef VTM_NET_SOCKET_SHARED_H_
#define VTM_NET_SOCKET_SHARED_H_

#ifdef __cplusplus
extern "C" {
#endif

/** TLS configuration */
struct vtm_socket_tls_cfg
{
	bool        enabled;     /**< true if TLS should be enabled */
	const char  *cert_file;  /**< full path to certificate in PEM format */
	const char  *key_file;   /**< full path to key in PEM format */
	const char  *ciphers;    /**< list of accepted ciphers */
};

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_SOCKET_SHARED_H_ */
