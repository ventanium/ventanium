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
	vtm_socket *sock, *client;
	size_t bytes_sent;
	size_t bytes_read;
	char buf[64];

	client = NULL;

	/* init network module */
	rc = vtm_module_network_init();
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* create socket */
	sock = vtm_socket_new(VTM_SOCK_FAM_IN4, VTM_SOCK_TYPE_STREAM);
	if (!sock) {
		vtm_err_print();
		goto end;
	}

	/* bind tcp port */
	rc = vtm_socket_bind(sock, "127.0.0.1", 5000);
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end;
	}

	/* listen on tcp port */
	rc = vtm_socket_listen(sock, 5);
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end;
	}

	printf("Waiting for TCP Connection on 127.0.0.1:5000\n");
	fflush(stdout);

	/* accept client connection */
	rc = vtm_socket_accept(sock, &client);
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end;
	}

	/* wait for ping */
	rc = vtm_socket_read(client, buf, sizeof(buf), &bytes_read);
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end;
	}

	printf("Received msg=%.*s\n", (int) bytes_read, buf);

	/* send pong */
	rc = vtm_socket_write(client, "Ping\n", strlen("Ping\n"), &bytes_sent);
	if (rc != VTM_OK)
		vtm_err_print();

end:
	/* free resources */
	vtm_socket_free(client);
	vtm_socket_free(sock);

	/* shutdown network module */
	vtm_module_network_end();

	return 0;
}
