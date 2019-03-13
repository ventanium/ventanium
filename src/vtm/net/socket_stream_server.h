/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file socket_stream_server.h
 *
 * @brief Connection based network server
 */

#ifndef VTM_NET_SOCKET_STREAM_SERVER_H_
#define VTM_NET_SOCKET_STREAM_SERVER_H_

#include <vtm/core/api.h>
#include <vtm/core/dataset.h>
#include <vtm/net/socket.h>
#include <vtm/net/socket_shared.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtm_socket_stream_srv vtm_socket_stream_srv;
struct vtm_socket_stream_srv_opts;

/**
 * Holds the user defined callbacks for a stream server.
 *
 * When a specific callback is not needed, then is MUST be set to NULL.
 *
 * Unless otherwise stated each callback function is called from a
 * worker-thread. When the server runs in single-threaded mode, the
 * functions are called from the main thread that started the server.
 */
struct vtm_socket_stream_srv_cbs
{
	/**
	 * This function is called when the socket of the server was successfully
	 * created and is ready to accept incoming connections.
	 *
	 * The function is called in the same thread that called
	 * vtm_socket_stream_srv_run().
	 *
	 * @param srv the server which is ready
	 * @param opts the options with which the server was started
	 */
	void (*server_ready)(vtm_socket_stream_srv *srv, struct vtm_socket_stream_srv_opts *opts);

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
	void (*worker_init)(vtm_socket_stream_srv *srv, vtm_dataset *wd);

	/**
	 * This function is called during shutdown phase of the server
	 * for each worker thread.
	 *
	 * @param srv the server
	 * @param wd the working dataset
	 */
	void (*worker_end)(vtm_socket_stream_srv *srv, vtm_dataset *wd);

	/**
	 * A new client has connected to the server.
	 *
	 * @param srv the server
	 * @param wd the working dataset
	 * @param client the connected socket representing the client connection
	 */
	void (*sock_connected)(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *client);

	/**
	 * A client connection was closed.
	 *
	 * @param srv the server
	 * @param wd the working dataset
	 * @param client the socket representing the client connection that was
	 *        closed
	 */
	void (*sock_disconnected)(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *client);

	/**
	 * A socket is rwady for reading data.
	 *
	 * @param srv the server
	 * @param wd the working dataset
	 * @param client the socket that has data available
	 */
	void (*sock_can_read)(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *client);

	/**
	 * A socket is ready for writing data.
	 *
	 * @param srv the server
	 * @param wd the working dataset
	 * @param client the socket that is ready
	 */
	void (*sock_can_write)(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *client);

	/**
	 * A socket is in an error state.
	 *
	 * @param srv the server
	 * @param wd the working dataset
	 * @param client the socket that is in an error state
	 */
	void (*sock_error)(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *client);
};

struct vtm_socket_stream_srv_opts
{
	/** The binding address and port */
	struct vtm_socket_addr addr;

	/**< TLS options if necessary */
	struct vtm_socket_tls_cfg tls;

	/** Callbacks to use */
	struct vtm_socket_stream_srv_cbs cbs;

	/** Length of queue for pending incoming connections */
	unsigned int backlog;

	/**
	 * Maximum number of socket events per iteration, should be at least equal
	 * or larger than the number of threads
	 */
	unsigned int events;

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
VTM_API vtm_socket_stream_srv* vtm_socket_stream_srv_new(void);

/**
 * Releases the server and all allocated resources.
 *
 * After this call the server pointer is no longer valid.
 *
 * @param srv the server which should be released
 */
VTM_API void vtm_socket_stream_srv_free(vtm_socket_stream_srv *srv);

/**
 * Retrieves the stored user data.
 *
 * @param srv the server
 * @return the previous saved user data
 * @return NULL if no data was stored
 */
VTM_API void* vtm_socket_stream_srv_get_usr_data(vtm_socket_stream_srv *srv);

/**
 * Stores arbitrary user data with the server.
 *
 * @param srv the server that should store the user data
 * @param data the data that should be stored
 */
VTM_API void vtm_socket_stream_srv_set_usr_data(vtm_socket_stream_srv *srv, void *data);

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
VTM_API int vtm_socket_stream_srv_run(vtm_socket_stream_srv *srv, struct vtm_socket_stream_srv_opts *opts);

/**
 * Stops the server.
 *
 * This method blocks until the server is gracefully shutdown.
 *
 * @param srv the server which should be stopped
 * @return VTM_OK if the shutdown was successful
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_stream_srv_stop(vtm_socket_stream_srv *srv);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_SOCKET_STREAM_SERVER_H_ */
