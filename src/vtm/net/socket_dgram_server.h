/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file socket_dgram_server.h
 *
 * @brief Datagram based network server
 */

#ifndef VTM_NET_SOCKET_DGRAM_SERVER_H_
#define VTM_NET_SOCKET_DGRAM_SERVER_H_

#include <vtm/core/api.h>
#include <vtm/core/buffer.h>
#include <vtm/core/dataset.h>
#include <vtm/net/socket.h>
#include <vtm/net/socket_shared.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtm_socket_dgram_srv vtm_socket_dgram_srv;
struct vtm_socket_dgram_srv_opts;

struct vtm_socket_dgram
{
	struct vtm_buf          buf;    /**< buffer containing the payload */
	struct vtm_socket_saddr saddr;  /**< the source address of the datagram */
};

/**
 * Holds the user defined callbacks for a datagram server.
 *
 * When a specific callback is not needed, then is MUST be set to NULL.
 *
 * Unless otherwise stated each callback function is called from a
 * worker-thread. When the server runs in single-threaded mode, the
 * functions are called from the main thread that started the server.
 */
struct vtm_socket_dgram_srv_cbs
{
	/**
	 * This function is called when the socket of the server was successfully
	 * created and is ready to receive incoming datagram packets.
	 *
	 * The function is called in the same thread that called
	 * vtm_socket_dgram_srv_run().
	 *
	 * @param srv the server which is ready
	 * @param opts the options with which the server was started
	 */
	void (*server_ready)(vtm_socket_dgram_srv *srv, struct vtm_socket_dgram_srv_opts *opts);

	/**
	 * This function is is called during startup phase of the server
	 * for each worker thread.
	 *
	 * @param srv the server
	 * @param wd an empty dataset set. The same dataset is later delivered to
	 *           this thread in the other callbacks. You can store things here
	 *           that may be used during processing of client messages,
	 *           for example a database connection.
	 */
	void (*worker_init)(vtm_socket_dgram_srv *srv, vtm_dataset *wd);

	/**
	 * This function is called during shutdown phase of the server
	 * for each worker thread.
	 *
	 * @param srv the server
	 * @param wd the working dataset
	 */
	void (*worker_end)(vtm_socket_dgram_srv *srv, vtm_dataset *wd);

	/**
	 * A new datagram was received.
	 *
	 * @param srv the server
	 * @param wd the working dataset
	 * @param dgram the received datagram
	 */
	void (*dgram_recv)(vtm_socket_dgram_srv *srv, vtm_dataset *wd, struct vtm_socket_dgram *dgram);
};

struct vtm_socket_dgram_srv_opts
{
	/** Binding address and port */
	struct vtm_socket_addr addr;

	/**
	 * Callbacks to use while running.
	 * At least the dgram_recv() callback MUST be set
	 */
	struct vtm_socket_dgram_srv_cbs cbs;

	/** Queue length for unprocessed datagrams */
	unsigned int queue_limit;

	/**
	 * Number of worker threads to use. Setting this value to zero
	 * lets the server run in single threaded mode.
	 */
	unsigned int threads;
};

/**
 * Creates a new server.
 *
 * @return the created server which can be used in the other functions
 * @return NULL if an error occured
 */
VTM_API vtm_socket_dgram_srv* vtm_socket_dgram_srv_new(void);

/**
 * Releases the server and all allocated resources.
 *
 * After this call the server pointer is no longer valid.
 *
 * @param srv the server which should be released
 */
VTM_API void vtm_socket_dgram_srv_free(vtm_socket_dgram_srv *srv);

/**
 * Retrieves the stored user data.
 *
 * @param srv the server
 * @return the previous saved user data
 * @return NULL if no data was stored
 */
VTM_API void* vtm_socket_dgram_srv_get_usr_data(vtm_socket_dgram_srv *srv);

/**
 * Stores arbitrary user data with the server.
 *
 * @param srv the server that should store the user data
 * @param data the data that should be stored
 */
VTM_API void vtm_socket_dgram_srv_set_usr_data(vtm_socket_dgram_srv *srv, void *data);

/**
 * Runs the server.
 *
 * This method blocks until the server is stopped or if an error occurs.
 *
 * @param srv the previously created server
 * @param opts the options for running the server
 * @return VTM_OK if the server has been started successfully and has
 *         already been shut down again
 * @return VTM_ERROR or other more specific error code when the server
 *         could not be started
 */
VTM_API int vtm_socket_dgram_srv_run(vtm_socket_dgram_srv *srv, struct vtm_socket_dgram_srv_opts *opts);

/**
 * Stops the server.
 *
 * This method blocks until the server is gracefully shutdown.
 *
 * @param srv the server which should be stopped
 * @return VTM_OK if the shutdown was successful
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_dgram_srv_stop(vtm_socket_dgram_srv *srv);

/**
 * Sends a datagram to the given address.
 *
 * @param srv the server that should send the message
 * @param buf the payload of the datagram
 * @param len the the length in bytes of the payload
 * @param saddr the destination address
 * @return VTM_OK if the datagram was sent successfully
 * @return VTM_E_IO_UNKNOWN or VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_dgram_srv_send(vtm_socket_dgram_srv *srv, void *buf, size_t len, const struct vtm_socket_saddr *saddr);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_SOCKET_DGRAM_SERVER_H_ */
