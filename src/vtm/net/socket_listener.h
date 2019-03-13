/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file socket_listener.h
 *
 * @brief Event based socket listener (epoll, kqueue, select)
 */

#ifndef VTM_NET_SOCKET_LISTENER_H_
#define VTM_NET_SOCKET_LISTENER_H_

#include <vtm/core/api.h>
#include <vtm/net/socket_event.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtm_socket_listener vtm_socket_listener;

/**
 * Creates a new socket listener.
 *
 * @param max_events maximum number of events this listener should
 *        return at once
 * @return a socket listener handle
 * @return NULL if an error occured
 */
VTM_API vtm_socket_listener* vtm_socket_listener_new(size_t max_events);

/**
 * Releases the socker listener and all allocated resources.
 *
 * @param li the target listener
 */
VTM_API void vtm_socket_listener_free(vtm_socket_listener *li);

/**
 * Registers a socket for receiving events.
 *
 * The socket hints (for example VTM_SOCK_HINT_NBL_READ) must
 * be set before the socket is registered.
 *
 * @return VTM_OK if the socket was successfully registered
 * @return VTM_E_MAX_REACHED if the maximum number of sockets this type of
 *         socket listener could handle is reached
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_listener_add(vtm_socket_listener *li, vtm_socket *sock);

/**
 * Unregisters a socket.
 *
 * @param li the listener where the socket should be removed from
 * @param sock the socket that should be removed
 * @return VTM_OK if the socket was successfully unregistered
 * @return VTM_ERROR if the socket was not registered
 */
VTM_API int vtm_socket_listener_remove(vtm_socket_listener *li, vtm_socket *sock);

/**
 * Marks the socket so that new events are received.
 *
 * @param li the corresponding listener
 * @param sock the socket that should be re-armed.
 * @return VTM_OK if call succeeded
 * @return VTM_ERROR if an error occcured
 */
VTM_API int vtm_socket_listener_rearm(vtm_socket_listener *li, vtm_socket *sock);

/**
 * Wait for new socket events.
 *
 * This call may block a certain time if no events are available.
 *
 * @param li the target listener
 * @param[out] events pointer to array of socket event pointers
 * @param[out] num_events number of events read
 * @return VTM_OK if the call succeed
 * @return VTM_ERROR if an error occcured
 */
VTM_API int vtm_socket_listener_run(vtm_socket_listener *li, struct vtm_socket_event **events, size_t *num_events);

/**
 * Interrupts the listener if he is blocked in waiting for events.
 *
 * @param li the listener that should be interrupted
 * @return VTM_OK if the call succeed
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_socket_listener_interrupt(vtm_socket_listener *li);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_SOCKET_LISTENER_H_ */
