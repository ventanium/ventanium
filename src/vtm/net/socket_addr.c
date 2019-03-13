/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "socket_addr.h"

#include <string.h> /* strcmp() */
#include <vtm/core/error.h>
#include <vtm/net/socket_addr_intl.h>

/* forward declaration */
static int vtm_socket_addr_get_info_ip4(const char *addr, struct vtm_socket_addr_info *info);
static int vtm_socket_addr_get_info_ip6(const char *addr, struct vtm_socket_addr_info *info);

int vtm_socket_addr_get_info(const char *addr, struct vtm_socket_addr_info *info)
{
	switch (info->sock_family) {
		case VTM_SOCK_FAM_IN4:
			return vtm_socket_addr_get_info_ip4(addr, info);

		case VTM_SOCK_FAM_IN6:
			return vtm_socket_addr_get_info_ip6(addr, info);
	}

	return VTM_ERROR;
}

static int vtm_socket_addr_get_info_ip4(const char *addr, struct vtm_socket_addr_info *info)
{
	int rc;

	rc = VTM_OK;

	if (strcmp(addr, VTM_SOCK_ADDR_LOCALHOST) == 0) {
		info->addr_type = VTM_SOCK_ADDR_TYPE_IPV4_LOOPBACK;
		goto end;
	}
	else if (strcmp(addr, VTM_SOCK_ADDR_IPV4_LOOPBACK) == 0) {
		info->addr_type = VTM_SOCK_ADDR_TYPE_IPV4_LOOPBACK;
		goto end;
	}
	else if (strcmp(addr, VTM_SOCK_ADDR_IPV4_ANY) == 0) {
		info->addr_type = VTM_SOCK_ADDR_TYPE_IPV4_ANY;
		goto end;
	}
	else {
		info->addr_type = VTM_SOCK_ADDR_TYPE_IPV4_SPEC;
		goto end;
	}

end:
	return rc;
}

static int vtm_socket_addr_get_info_ip6(const char *addr, struct vtm_socket_addr_info *info)
{
	int rc;

	rc = VTM_OK;

	if (strcmp(addr, VTM_SOCK_ADDR_LOCALHOST) == 0) {
		info->addr_type = VTM_SOCK_ADDR_TYPE_IPV6_LOOPBACK;
		goto end;
	}
	else if (strcmp(addr, VTM_SOCK_ADDR_IPV6_LOOPBACK) == 0) {
		info->addr_type = VTM_SOCK_ADDR_TYPE_IPV6_LOOPBACK;
		goto end;
	}
	else if (strcmp(addr, VTM_SOCK_ADDR_IPV6_ANY) == 0) {
		info->addr_type = VTM_SOCK_ADDR_TYPE_IPV6_ANY;
		goto end;
	}
	else {
		info->addr_type = VTM_SOCK_ADDR_TYPE_IPV6_SPEC;
		goto end;
	}

end:
	return rc;
}
