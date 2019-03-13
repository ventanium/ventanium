/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/net/socket_addr.h>

#include <string.h> /* memset() */
#include <vtm/core/error.h>
#include <vtm/core/format.h>
#include <vtm/core/math.h>
#include <vtm/sys/base/net/socket_util_intl.h>

#ifdef VTM_HAVE_POSIX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#elif VTM_SYS_WINDOWS
#include <winsock2.h>
#endif

#ifdef __MINGW32__
extern const char* inet_ntop(int af, const void *src,
	char *dst, size_t size);
#endif

int vtm_socket_os_addr_build(struct vtm_socket_saddr *saddr, struct vtm_socket_addr *from)
{
	int rc;
	int sockfam;
	struct addrinfo hints;
	struct addrinfo *info;
	char portbuf[VTM_FMT_CHARS_INT32];

	rc = vtm_socket_util_convert_family(from->family, &sockfam);
	if (rc != VTM_OK)
		return rc;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = sockfam;
	info = NULL;

	rc = vtm_fmt_uint(portbuf, from->port);
	portbuf[rc] = '\0';

	rc = getaddrinfo(from->host, portbuf, &hints, &info);
	if (rc != 0)
		return vtm_socket_util_error(NULL);

	if (!info)
		return VTM_ERROR;

	switch (from->family) {
		case VTM_SOCK_FAM_IN4:
			saddr->addr.in4 = *(struct sockaddr_in*) info->ai_addr;
			break;

		case VTM_SOCK_FAM_IN6:
			saddr->addr.in6 = *(struct sockaddr_in6*) info->ai_addr;
			break;
	}

	vtm_socket_util_prepare_saddr(from->family, saddr);

	freeaddrinfo(info);

	return VTM_OK;
}

int vtm_socket_os_addr_convert(struct vtm_socket_saddr *saddr, enum vtm_socket_family *fam, char *host_buf, size_t len, unsigned int *port)
{
	switch (saddr->addr.sa.sa_family) {
		case AF_INET:
			if (host_buf) {
#ifdef VTM_SYS_WINDOWS
				if (getnameinfo(&(saddr->addr.sa), saddr->len,
					host_buf, (DWORD) VTM_MIN(len, 4096), NULL,0, NI_NUMERICHOST) != 0) {
					return VTM_ERROR;
				}
#else
				if (!inet_ntop(AF_INET, &(saddr->addr.in4.sin_addr), host_buf, len))
					return vtm_socket_util_error(NULL);
#endif
			}

			if (fam)
				*fam = VTM_SOCK_FAM_IN4;
			if (port)
				*port = ntohs(saddr->addr.in4.sin_port);
			return VTM_OK;

		case AF_INET6:
			if (host_buf) {
#ifdef VTM_SYS_WINDOWS
				if (getnameinfo(&(saddr->addr.sa), saddr->len,
					host_buf, (DWORD) VTM_MIN(len, 4096), NULL,0, NI_NUMERICHOST) != 0) {
					return VTM_ERROR;
				}
#else
				if (!inet_ntop(AF_INET6, &(saddr->addr.in6.sin6_addr), host_buf, len))
					return vtm_socket_util_error(NULL);
#endif
			}
			if (fam)
				*fam = VTM_SOCK_FAM_IN6;
			if (port)
				*port = ntohs(saddr->addr.in6.sin6_port);
			return VTM_OK;

		default:
			break;
	}

	return vtm_err_set(VTM_E_NOT_SUPPORTED);
}
