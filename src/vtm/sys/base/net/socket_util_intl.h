/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_SYS_BASE_NET_SOCKET_UTIL_INTL_H_
#define VTM_SYS_BASE_NET_SOCKET_UTIL_INTL_H_

#include <vtm/net/socket_intl.h>

#ifdef __cplusplus
extern "C" {
#endif

int vtm_socket_util_block_sigpipe(vtm_sys_socket_t fd);
int vtm_socket_util_convert_family(enum vtm_socket_family fam, int *out_fam);
int vtm_socket_util_convert_type(int type, int *out_type);
int vtm_socket_util_prepare_saddr(enum vtm_socket_family fam, struct vtm_socket_saddr *saddr);

int vtm_socket_util_bind(struct vtm_socket *sock, const char *addr, unsigned int port);
int vtm_socket_util_listen(struct vtm_socket *sock, unsigned int backlog);
int vtm_socket_util_connect(struct vtm_socket *sock, const char *host, unsigned int port);
int vtm_socket_util_get_remote_addr(struct vtm_socket *sock, struct vtm_socket_saddr *saddr);

int vtm_socket_util_set_opt(struct vtm_socket *sock, int opt, const void *val, size_t len);
int vtm_socket_util_get_opt(struct vtm_socket *sock, int opt, void *val, size_t len);

int vtm_socket_util_set_nonblocking(vtm_sys_socket_t fd, bool enabled);
int vtm_socket_util_set_recv_timeout(vtm_sys_socket_t fd, struct vtm_socket *sock, unsigned long millis);

int vtm_socket_util_error(struct vtm_socket *sock);
int vtm_socket_util_read_error(struct vtm_socket *sock);
int vtm_socket_util_write_error(struct vtm_socket *sock);

#ifdef __cplusplus
}
#endif

#endif /* VTM_SYS_BASE_NET_SOCKET_UTIL_INTL_H_ */
