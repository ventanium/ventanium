/*
 * Copyright (C) 2018-2023 Matthias Benkendorf
 */

/**
 * @file socket.h
 *
 * @brief Socket commmunication
 */

#ifndef VTM_NET_SOCKET_H_
#define VTM_NET_SOCKET_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/net/network.h>
#include <vtm/net/socket_addr.h>
#include <vtm/net/socket_spec.h>

#ifdef __cplusplus
extern "C" {
#endif

/* status */
#define VTM_SOCK_STAT_DEFAULT                            0
#define VTM_SOCK_STAT_ERR                         (1 <<  0)
#define VTM_SOCK_STAT_CLOSED                      (1 <<  1)
#define VTM_SOCK_STAT_HUP                         (1 <<  2)
#define VTM_SOCK_STAT_READ_AGAIN                  (1 <<  3)
#define VTM_SOCK_STAT_READ_AGAIN_WHEN_WRITEABLE   (1 <<  4)
#define VTM_SOCK_STAT_WRITE_AGAIN                 (1 <<  5)
#define VTM_SOCK_STAT_WRITE_AGAIN_WHEN_READABLE   (1 <<  6)
#define VTM_SOCK_STAT_READ_LOCKED                 (1 <<  7)
#define VTM_SOCK_STAT_WRITE_LOCKED                (1 <<  8)
#define VTM_SOCK_STAT_FREE_ON_UNREF               (1 <<  9)
#define VTM_SOCK_STAT_NONBLOCKING                 (1 << 10)
#define VTM_SOCK_STAT_NBL_READ                    (1 << 11)  /**< Non-blocking read */
#define VTM_SOCK_STAT_NBL_WRITE                   (1 << 12)  /**< Non-blocking write */
#define VTM_SOCK_STAT_NBL_AUTO                    (1 << 13)  /**< Non-blocking read or write, automatically switched */

/* shutdown */
#define VTM_SOCK_SHUT_RD                   1  /**< Shutdown read-side */
#define VTM_SOCK_SHUT_WR                   2  /**< Shutdown write-side */

/* options */
#define VTM_SOCK_OPT_KEEPALIVE             1  /**< expects bool */
#define VTM_SOCK_OPT_NONBLOCKING           2  /**< expects bool */
#define VTM_SOCK_OPT_RECV_TIMEOUT          3  /**< expects unsigned long, value is milliseconds */
#define VTM_SOCK_OPT_SEND_TIMEOUT          4  /**< expects unsigned long, value is milliseconds */
#define VTM_SOCK_OPT_TCP_KEEPALIVE_IDLE    5  /**< expects int, value is seconds */
#define VTM_SOCK_OPT_TCP_KEEPALIVE_INTVL   6  /**< expects int, value is seconds */
#define VTM_SOCK_OPT_TCP_KEEPALIVE_PROBES  7  /**< expects int, value is count */
#define VTM_SOCK_OPT_TCP_NODELAY           8  /**< expects bool */

/* default TLS ciphers */
#define VTM_SOCKET_TLS_DEFAULT_CIPHERS                              \
	"ECDH+AESGCM:ECDH+CHACHA20:DH+AESGCM:ECDH+AES256:DH+AES256:"    \
	"ECDH+AES128:DH+AES:RSA+AESGCM:RSA+AES:!aNULL:!MD5:!DSS"

struct vtm_socket_tls_opts
{
	bool is_server;
	bool no_cert_check;
	const char *ca_file;
	const char *cert_file;
	const char *key_file;
	const char *ciphers;
};

typedef struct vtm_socket vtm_socket;

/**
 * Creates a new plain (non-encrypted) socket.
 *
 * @see socket_spec.h
 * @param fam the socket family
 * @param int the type of socket
 * @return a handle to the created socket
 * @return NULL if an error occured
 */
VTM_API vtm_socket* vtm_socket_new(enum vtm_socket_family fam, int type);

/**
 * Creates a new stream-based TLS socket.
 *
 * @param fam the socket family
 * @param opts TLS options
 * @return a handle to the created socket
 * @return NULL if an error occured
 */
VTM_API vtm_socket* vtm_socket_tls_new(enum vtm_socket_family fam, struct vtm_socket_tls_opts *opts);

/**
 * Releases the socket and all allocated resources.
 *
 * After this call the socket pointer is no longer valid.
 *
 * @param sock the socket that should be released
 */
VTM_API void vtm_socket_free(vtm_socket *sock);

/**
 * Makes the socket threadsafe.
 *
 * @param sock the socket that should be made threadsafe
 * @return VTM_OK if the socket is now threadsafe
 * @return VTM_E_MALLOC if the necessary mutex object could not be created
 */
VTM_API int vtm_socket_make_threadsafe(vtm_socket *sock);

/**
 * Stores arbitrary user data with the socket.
 *
 * @param sock the socket that should store the user data
 * @param data the data that should be stored
 */
VTM_API void vtm_socket_set_usr_data(vtm_socket *sock, void *data);

/**
 * Retrieves the stored user data.
 *
 * @param sock the socket
 * @return the previous saved user data
 * @return NULL if no data was stored
 */
VTM_API void* vtm_socket_get_usr_data(vtm_socket *sock);

/**
 * Gets the socket family of the given socket.
 *
 * @param sock the socket
 * @return the socket family of the socket
 */
VTM_API enum vtm_socket_family vtm_socket_get_family(vtm_socket *sock);

/**
 * Gets the type of the given socket.
 *
 * @param sock the socket
 * @return the type of the socket
 */
VTM_API int vtm_socket_get_type(vtm_socket *sock);

/**
 * Binds the socket the given address and port.
 *
 * @param addr the address
 * @param port the port, must be in range 0-65535
 * @return VTM_OK if the bind operation was successfull
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_bind(vtm_socket *sock, const char *addr, unsigned int port);

/**
 * Starts listening on the given socket.
 *
 * Marks the socket as passive, so that incoming connections can be accepted
 * with vtm_socket_accept().
 *
 * @param sock the socket that should start listening
 * @param backlog maximum number of pending connections that are not accepted
 *        yet
 * @return VTM_OK if the call succeeded
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_listen(vtm_socket *sock, unsigned int backlog);

/**
 * Accepts a new incoming connection.
 *
 * If the listening socket is not in non-blocking mode, this call blocks
 * until a new connection was accepted.
 *
 * @param sock the listening socket
 * @param[out] client pointer to connected socket which represents the accepted
 *             client connection
 * @return VTM_OK if the call succeeded and a new client connection was
 *         accepted
 * @return VTM_E_IO_AGAIN if the socket is is in non-blocking mode and there
 *         was no pending incoming connection.
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_accept(vtm_socket *sock, vtm_socket **client);

/**
 * Connects the socket to given address and port.
 *
 * @param host either a hostname or an ip address
 * @param port the port number
 * @return VTM_OK if the call succeeded and the socket is now connected
 * @return VTM_E_NOT_SUPPORTED if the operation is not supported, for example
 *         you cannot connect a datagram based socket
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_connect(vtm_socket *sock, const char *host, unsigned int port);

/**
 * Shuts down the reading or writing side of a socket.
 *
 * @param sock the socket that should shutdown
 * @param dir the direction or a combination of multiple directions that should
 *        be shut down
 * @return VTM_OK if the call succeeded
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_shutdown(vtm_socket *sock, int dir);

/**
 * Closes a socket.
 *
 * @param sock the socket that should be closed
 * @return VTM_OK if the call succeeded
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_close(vtm_socket *sock);

/**
 * Writes data to the socket.
 *
 * If the socket is not in non-blocking mode, this call blocks
 * until all data is written.
 *
 * @param sock the socket where the data should be written to
 * @param src pointer to the data that should be written
 * @param len length of data in bytes
 * @param[out] out_written number of bytes that were successfully written
 * @return VTM_OK if the call succeeded
 * @return VTM_E_IO_AGAIN if not all data could be written at once
 * @return VRM_E_IO_CLOSED if the connection was closed
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_write(vtm_socket *sock, const void *src, size_t len, size_t *out_written);

/**
 * Reads data from the socket.
 *
 * If the socket is not in non-blocking mode, this call blocks
 * until some data is available to read.
 *
 * @param sock the socket where the data should be read from
 * @param[out] buf the buffer where the data will be stored
 * @param len the size of the buffer in bytes
 * @param[out] out_read the number of bytes that were written to the buffer
 * @return VTM_OK if the call succeeded
 * @return VTM_E_IO_AGAIN if there was no data available
 * @return VRM_E_IO_CLOSED if the connection was closed
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_read(vtm_socket *sock, void *buf, size_t len, size_t *out_read);

/**
 * Receive a datagram.
 *
 * If the socket is not in non-blocking mode, this call blocks
 * until a datagram is available to read.
 *
 * @param sock the socket where the datagram should be read from
 * @param[out] buf the buffer where the data will be stored
 * @param maxlen size of the buffer in bytes
 * @param[out] out_recv number of bytes that were written to the buffer
 * @param saddr the source address of the datagram
 * @return VTM_OK if the call succeeded
 * @return VTM_E_IO_AGAIN if there was no data available
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_dgram_recv(vtm_socket *sock, void *buf, size_t maxlen, size_t *out_recv, struct vtm_socket_saddr *saddr);

/**
 * Send a datagram.
 *
 * @param sock the socket that should send the datagram
 * @param buf the buffer with the payload
 * @param len length of payload in bytes
 * @param[out] out_send number of bytes that were sent
 * @param saddr the destination address of the datagram
 * @return VTM_OK if the call succeeded
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_dgram_send(vtm_socket *sock, const void *buf, size_t len, size_t *out_send, const struct vtm_socket_saddr *saddr);

/**
 * Adds one or a combination of state flags.
 *
 * @param sock the socket where the state flags should be set
 * @param flags one or more OR-combined flags
 */
VTM_API void vtm_socket_set_state(vtm_socket *sock, unsigned int flags);

/**
 * Removes one or a combination of state flags.
 *
 * @param sock the socket where the state flags should be removed
 * @param flags one or more OR-combined flags
 */
VTM_API void vtm_socket_unset_state(vtm_socket *sock, unsigned int flags);

/**
 * Retrieves the current socket state.
 *
 * @param sock the socket
 * @return the current state
 */
VTM_API unsigned int vtm_socket_get_state(vtm_socket *sock);

/**
 * Set socket option.
 *
 * @param sock the socket where the option should be set
 * @param opt the option that should be set
 * @param val the value that should be set
 * @param len length of the value in bytes
 * @return VTM_OK if the option was successfully set
 * @return VTM_E_NOT_SUPPORTED if the option is not supported
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_set_opt(vtm_socket *sock, int opt, const void *val, size_t len);

/**
 * Retrieve current value of a socket option.
 *
 * @param sock the socket where the option should be retrieved
 * @param opt the option that should be read
 * @param[out] val the current value of the option
 * @param len length of value in bytes
 * @return VTM_OK if the option was successfully read
 * @return VTM_E_NOT_SUPPORTED if the option is not supported
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_get_opt(vtm_socket *sock, int opt, void *val, size_t len);

/**
 * Reads the remote peer address of a connected socket.
 *
 * @param sock a connected socket
 * @param[out] addr the buffer where the address is stored
 * @return VTM_OK if the peer address was sucessfully read
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_get_remote_addr(vtm_socket *sock, struct vtm_socket_saddr *addr);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_SOCKET_H_ */
