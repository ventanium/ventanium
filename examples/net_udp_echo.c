/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <stdio.h>
#include <string.h>
#include <vtm/core/error.h>
#include <vtm/net/socket.h>

int main(void)
{
	int rc;
	vtm_socket *sock;
	struct vtm_socket_saddr client_saddr;
	size_t bytes_sent;
	size_t bytes_read;
	char buf[64];

	/* init network module */
	rc = vtm_module_network_init();
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* create socket */
	sock = vtm_socket_new(VTM_SOCK_FAM_IN4, VTM_SOCK_TYPE_DGRAM);
	if (!sock) {
		vtm_err_print();
		goto end;
	}

	/* bind udp port */
	rc = vtm_socket_bind(sock, "127.0.0.1", 5000);
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end;
	}

	printf("Waiting for UDP packet on 127.0.0.1:5000\n");
	fflush(stdout);

	/* wait for ping */
	rc = vtm_socket_dgram_recv(sock, buf, sizeof(buf), &bytes_read, &client_saddr);
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end;
	}

	printf("Received msg=%.*s\n", (int) bytes_read, buf);

	/* send pong */
	rc = vtm_socket_dgram_send(sock, "Ping\n", strlen("Ping\n"), &bytes_sent, &client_saddr);
	if (rc != VTM_OK)
		vtm_err_print();

end:
	/* free resources */
	vtm_socket_free(sock);

	/* shutdown network module */
	vtm_module_network_end();

	return 0;
}
