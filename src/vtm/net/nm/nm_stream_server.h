/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file nm_stream_server.h
 *
 * @brief Connection based network message server
 */

#ifndef VTM_NET_NM_NM_STREAM_SERVER_H_
#define VTM_NET_NM_NM_STREAM_SERVER_H_

#include <vtm/core/api.h>
#include <vtm/core/dataset.h>
#include <vtm/net/network.h>
#include <vtm/net/socket_addr.h>
#include <vtm/net/socket_shared.h>
#include <vtm/net/socket_spec.h>
#include <vtm/net/nm/nm_stream_connection.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtm_nm_stream_srv vtm_nm_stream_srv;
struct vtm_nm_stream_srv_opts;

/**
 * Holds the user defined callbacks for a NM stream server.
 *
 * When a specific callback is not needed, then is MUST be set to NULL.
 *
 * Unless otherwise stated each callback function is called from a
 * worker-thread. When the server runs in single-threaded mode, the
 * functions are called from the main thread that started the server.
 */
struct vtm_nm_stream_srv_cbs
{
	/**
	 * This function is called when the socket of the server was successfully
	 * created and is ready to accept incoming connections.
	 *
	 * The function is called in the same thread that called
	 * vtm_nm_stream_srv_run()
	 *
	 * @param srv the server which is ready
	 * @param opts the options of the server
	 */
	void (*server_ready)(vtm_nm_stream_srv *srv, struct vtm_nm_stream_srv_opts *opts);

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
	void (*worker_init)(vtm_nm_stream_srv *srv, vtm_dataset *wd);

	/**
	 * This function is called during shutdown phase of the server
	 * for each worker thread.
	 *
	 * @param srv the server
	 * @param wd the working dataset
	 */
	void (*worker_end)(vtm_nm_stream_srv *srv, vtm_dataset *wd);

	/**
	 * A new client has connected to the server.
	 *
	 * @param srv the server
	 * @param wd the working dataset
	 * @param con the connection associated with the new client
	 */
	void (*client_connect)(vtm_nm_stream_srv *srv, vtm_dataset *wd, vtm_nm_stream_con *con);

	/**
	 * A client connection was closed.
	 *
	 * @param srv the server
	 * @param wd the working dataset
	 * @param con the connection that was closed
	 */
	void (*client_disconnect)(vtm_nm_stream_srv *srv, vtm_dataset *wd, vtm_nm_stream_con *con);

	/**
	 * A new message was received.
	 *
	 * @param srv the server
	 * @param wd the working dataset
	 * @param con the connection that sent the msg
	 * @param msg the received message
	 */
	void (*client_msg)(vtm_nm_stream_srv *srv, vtm_dataset *wd, vtm_nm_stream_con *con, vtm_dataset *msg);
};

/**
 * Defines the options with which the server runs
 */
struct vtm_nm_stream_srv_opts
{
	/** The binding address and port */
	struct vtm_socket_addr addr;

	/** TLS options */
	struct vtm_socket_tls_cfg tls;

	/**
	 * Callbacks to use while running.
	 * At least the client_msg callback MUST be set.
	 */
	struct vtm_nm_stream_srv_cbs cbs;

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
VTM_API vtm_nm_stream_srv* vtm_nm_stream_srv_new(void);

/**
 * Frees the server and all allocated resources.
 *
 * @param srv the server which should be freed
 */
VTM_API void vtm_nm_stream_srv_free(vtm_nm_stream_srv *srv);

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
VTM_API int vtm_nm_stream_srv_run(vtm_nm_stream_srv *srv, struct vtm_nm_stream_srv_opts *opts);

/**
 * Stops the server.
 *
 * This method blocks until the server is gracefully shutdown.
 *
 * @param srv the server which should be stopped
 * @return VTM_OK if the shutdown was successful
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_nm_stream_srv_stop(vtm_nm_stream_srv *srv);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_NM_NM_STREAM_SERVER_H_ */
