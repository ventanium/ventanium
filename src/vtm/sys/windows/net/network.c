/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/net/network.h>

#include <winsock2.h>
#include <vtm/core/error.h>

int vtm_module_network_init(void)
{
	int rc;
	WSADATA wsa;
	
	rc = WSAStartup(MAKEWORD(2,2), &wsa);
	if (rc != 0)
		return VTM_ERROR;
	
	return VTM_OK;
}

void vtm_module_network_end(void)
{
	WSACleanup();
}
