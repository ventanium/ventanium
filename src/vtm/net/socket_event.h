/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file socket_event.h
 *
 * @brief Socket event definitions
 */

#ifndef VTM_NET_SOCKET_EVENT_H_
#define VTM_NET_SOCKET_EVENT_H_

#include <vtm/core/api.h>
#include <vtm/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/* events */
#define VTM_SOCK_EVT_READ     1    /**< socket is available for read operation */
#define VTM_SOCK_EVT_WRITE    2    /**< socket is available for write operation */
#define VTM_SOCK_EVT_CLOSED   4    /**< socket is closed */
#define VTM_SOCK_EVT_ERROR    8    /**< socket is in error state */

struct vtm_socket_event
{
	vtm_socket    *sock;           /**< socket that received the event */
	unsigned int  events;          /**< event types */
};

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_SOCKET_EVENT_H_ */
