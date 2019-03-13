/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_NET_SOCKET_INTL_H_
#define VTM_NET_SOCKET_INTL_H_

#include <vtm/core/api.h>
#include <vtm/core/flag.h>
#include <vtm/core/types.h>
#include <vtm/net/socket.h>
#include <vtm/util/mutex.h>

#if defined(VTM_SYS_UNIX) || defined(VTM_SYS_WINDOWS)
#include <vtm/sys/base/net/socket_types_intl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define VTM_SOCK_FD(SOCK)                       (SOCK)->fd
#define vtm_socket_set_state_intl(SOCK, FLAGS)  vtm_flag_set((SOCK)->state, (FLAGS))

struct vtm_socket_vtable
{
	void (*vtm_socket_free)(struct vtm_socket *sock);

	int (*vtm_socket_bind)(struct vtm_socket *sock, const char *addr, unsigned int port);
	int (*vtm_socket_listen)(struct vtm_socket *sock, unsigned int backlog);
	int (*vtm_socket_accept)(struct vtm_socket *sock, struct vtm_socket **client);

	int (*vtm_socket_connect)(struct vtm_socket *sock, const char *host, unsigned int port);
	int (*vtm_socket_shutdown)(struct vtm_socket *sock, int dir);
	int (*vtm_socket_close)(struct vtm_socket *sock);

	int (*vtm_socket_write)(struct vtm_socket *sock, const void *src, size_t len, size_t *out_written);
	int (*vtm_socket_read)(struct vtm_socket *sock, void *buf, size_t len, size_t *out_read);

	int (*vtm_socket_dgram_recv)(struct vtm_socket *sock, void *buf, size_t maxlen, size_t *out_recv, struct vtm_socket_saddr *saddr);
	int (*vtm_socket_dgram_send)(struct vtm_socket *sock, const void *buf, size_t len, size_t *out_send, const struct vtm_socket_saddr *saddr);

	int (*vtm_socket_set_opt)(struct vtm_socket *sock, int opt, const void *val, size_t len);
	int (*vtm_socket_get_opt)(struct vtm_socket *sock, int opt, void *val, size_t len);

	int (*vtm_socket_get_remote_addr)(struct vtm_socket *sock, struct vtm_socket_saddr *saddr);
};

struct vtm_socket
{
	vtm_sys_socket_t          fd;
	enum vtm_socket_family    family;
	int                       type;

	vtm_mutex                 *mtx;
	unsigned int              state;
	uint8_t                   refcount;
	void                      *info;
	void                      *usr_data;

	struct vtm_socket_vtable  *vtable;

	/* stream server */
	void *stream_srv;
	int (*vtm_socket_update_stream_srv)(void *stream_srv, struct vtm_socket *sock);
};

int  vtm_socket_base_init(struct vtm_socket *sock);
void vtm_socket_lock(struct vtm_socket *sock);
void vtm_socket_unlock(struct vtm_socket *sock);

void vtm_socket_ref(struct vtm_socket *sock);
void vtm_socket_unref(struct vtm_socket *sock);
void vtm_socket_enable_free_on_unref(struct vtm_socket *sock);

int vtm_socket_update_srv(struct vtm_socket *sock);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_SOCKET_INTL_H_ */
