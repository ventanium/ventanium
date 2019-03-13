/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file http_server.h
 *
 * @brief HTTP server
 */

#ifndef VTM_NET_HTTP_HTTP_SERVER_H_
#define VTM_NET_HTTP_HTTP_SERVER_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/net/socket.h>
#include <vtm/net/socket_addr.h>
#include <vtm/net/socket_shared.h>
#include <vtm/net/http/http.h>
#include <vtm/net/http/http_context.h>
#include <vtm/net/http/http_error.h>
#include <vtm/net/http/http_memory.h>
#include <vtm/net/http/http_request.h>
#include <vtm/net/http/http_response.h>
#include <vtm/net/http/ws_connection.h>
#include <vtm/net/http/ws_message.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtm_http_srv vtm_http_srv;
struct vtm_http_srv_opts;

/**
 * Holds the user defined callbacks for an HTTP server.
 *
 * When a specific callback is not needed, then is MUST be set to NULL.
 *
 * Unless otherwise stated each callback function is called from a
 * worker-thread. When the server runs in single-threaded mode, the
 * functions are called from the main thread that started the server.
 */
struct vtm_http_srv_cbs
{
	/**
	 * This function is called when the HTTP server was successfully
	 * created and is ready to accept incoming connections.
	 *
	 * The function is called in the same thread that called
	 * vtm_http_srv_run()
	 *
	 * @param srv the server that is ready
	 * @param opts the options with which the server was startet
	 */
	void (*server_ready)(vtm_http_srv *srv, struct vtm_http_srv_opts *opts);

	/**
	 * This function is is called during startup phase of the HTTP server
	 * for each worker thread.
	 *
	 * @param srv the server
	 * @param ctx the context with the global HTTP memory object and the
	 *        per-thread dataset
	 */
	void (*worker_init)(struct vtm_http_ctx *ctx);

	/**
	 * This function is called during shutdown phase of the HTTP server
	 * for each worker thread.
	 *
	 * @param srv the server
	 * @param ctx the context with the global HTTP memory object and the
	 *        per-thread dataset
	 */
	void (*worker_end)(struct vtm_http_ctx *ctx);

	/**
	 * Called for each HTTP request.
	 *
	 * @param ctx the context
	 * @param req the request details
	 * @param res response handle for sending back the HTTP response
	 */
	void (*http_request)(struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res);

	/**
	 * Called for each new WebSocket connection.
	 *
	 * @param ctx the context
	 * @param con the client connection object
	 */
	void (*ws_connect)(struct vtm_http_ctx *ctx, vtm_ws_con *con);

	/**
	 * Called when a new WebSocket message was received.
	 *
	 * @param ctx the context
	 * @param msg the received message
	 */
	void (*ws_message)(struct vtm_http_ctx *ctx, struct vtm_ws_msg *msg);

	/**
	 * Called when a WebSocket connection was closed.
	 *
	 * @param ctx the context
	 * @param con the connection that was closed
	 */
	void (*ws_close)(struct vtm_http_ctx *ctx, vtm_ws_con *con);
};

struct vtm_http_srv_opts
{
	/**
	 * Callbacks to use while running.
	 * At least the http_request callback MUST be set.
	 */
	struct vtm_http_srv_cbs cbs;

	/** TLS options */
	struct vtm_socket_tls_cfg   tls;

	/** the binding address for the TCP socket */
	const char *host;

	/** the binding port for the TCP socket */
	unsigned int port;

	/** backlog for incoming connections */
	unsigned int backlog;

	/** number of socket events per call */
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
VTM_API vtm_http_srv* vtm_http_srv_new(void);

/**
 * Releases the server and all allocated resources.
 *
 * Aftert this call the server pointer is no longer valid.
 *
 * @param srv the server that should be released
 */
VTM_API void vtm_http_srv_free(vtm_http_srv *srv);

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
VTM_API int vtm_http_srv_run(vtm_http_srv *srv, struct vtm_http_srv_opts *opts);

/**
 * Stops the server.
 *
 * This method blocks until the server is gracefully shutdown.
 *
 * @param srv the server which should be stopped
 * @return VTM_OK if the shutdown was successful
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_http_srv_stop(vtm_http_srv *srv);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_SERVER_H_ */
