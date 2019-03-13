/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <stdio.h>
#include <string.h>
#include <vtm/core/error.h>
#include <vtm/net/socket_stream_server.h>
#include <vtm/util/signal.h>
#include <vtm/util/thread.h>

vtm_socket_stream_srv *srv;
char *response;
size_t response_len;

void stop_server(int psig)
{
	vtm_signal_safe_puts("Stopping server...\n");
	vtm_socket_stream_srv_stop(srv);
}

void server_ready(vtm_socket_stream_srv *srv, struct vtm_socket_stream_srv_opts *opts)
{
	printf("TCP Server listening at: %s:%u\n", opts->addr.host, opts->addr.port);
	fflush(stdout);
}

void print_send_details(int rc, size_t bytes_written)
{
	switch (rc) {
		case VTM_OK:
			printf("sent complete response with %ld bytes\n", (long) bytes_written);
			break;

		case VTM_E_IO_AGAIN:
			printf("sent partial response with %ld bytes\n", (long) bytes_written);
			break;

		default:
			printf("unknown error while sending response: %d\n", rc);
			break;
	}
}

void worker_init(vtm_socket_stream_srv *srv, vtm_dataset *wd)
{
	printf("[%lu] Worker started\n", vtm_thread_get_current_id());
}

void worker_end(vtm_socket_stream_srv *srv, vtm_dataset *wd)
{
	printf("[%lu] Worker finished\n", vtm_thread_get_current_id());
}

void client_connected(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *client)
{
	int rc;
	struct vtm_socket_saddr saddr;
	char addr[VTM_SOCK_ADDR_BUF_LEN];
	unsigned int port;
	enum vtm_socket_family fam;

	printf("[%lu] Client connected: ", vtm_thread_get_current_id());

	rc = vtm_socket_get_remote_addr(client, &saddr);
	if (rc != VTM_OK) {
		vtm_err_print();
		return;
	}

	rc = vtm_socket_os_addr_convert(&saddr, &fam, addr, sizeof(addr), &port);
	if (rc != VTM_OK) {
		vtm_err_print();
		return;
	}

	printf("%s:%d\n",addr, port);

	/*
	 * Enable automatic switching between non-blocking reading
	 * and non-bolocking writing.
	 */
	vtm_socket_set_state(client, VTM_SOCK_STAT_NBL_AUTO);
}

void client_disconnected(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *client)
{
	printf("[%lu] Client disconnected\n", vtm_thread_get_current_id());
}

void client_can_read(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *client)
{
	int rc;
	char buf[1024];
	size_t bytes_read, bytes_written, *index;

	printf("[%lu] Client can read.. ", vtm_thread_get_current_id());

	rc = vtm_socket_read(client, buf, sizeof(buf), &bytes_read);
	switch (rc) {
		case VTM_OK:
			printf("read %d bytes ", (int) bytes_read);;
			break;

		case VTM_E_IO_CLOSED:
			printf("connection closed\n");
			return;

		default:
			printf("unknown error: %d\n", rc);
			return;
	}

	/* try to write big response */
	rc = vtm_socket_write(client, response, response_len, &bytes_written);
	print_send_details(rc, bytes_written);

	if (rc == VTM_E_IO_AGAIN) {
		index = malloc(sizeof(size_t));
		if (!index)
			vtm_err_oom();

		*index = bytes_written;
		vtm_socket_set_usr_data(client, index);
	}
}

void client_can_write(vtm_socket_stream_srv *srv, vtm_dataset *wd, vtm_socket *client)
{
	int rc;
	size_t bytes_written, *index;

	printf("[%lu] Client can write.. ", vtm_thread_get_current_id());

	index = vtm_socket_get_usr_data(client);
	rc = vtm_socket_write(client, response + *index, response_len - *index, &bytes_written);
	print_send_details(rc, bytes_written);
	switch (rc) {
		case VTM_E_IO_AGAIN:
			*index += bytes_written;
			break;

		default:
			free(index);
			vtm_socket_set_usr_data(client, NULL);
	}
}

int main(void)
{
	int rc;
	struct vtm_socket_stream_srv_opts opts;

	/* init 8 MB response */
	response_len = 8 * 1024 * 1024;
	response = malloc(response_len);
	if (!response)
		return EXIT_FAILURE;
	memset(response, 'A', response_len);

	/* register signal handler */
	vtm_signal_set_handler(VTM_SIG_INT, stop_server);

	/* init network module */
	rc = vtm_module_network_init();
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* prepare options */
	memset(&opts, 0, sizeof(opts));
	opts.addr.family = VTM_SOCK_FAM_IN4;
	opts.addr.host = "127.0.0.1";
	opts.addr.port = 5000;
	opts.backlog = 5;
	opts.events = 8;
	opts.threads = 2;
	opts.cbs.server_ready = server_ready;
	opts.cbs.worker_init = worker_init;
	opts.cbs.worker_end = worker_end;
	opts.cbs.sock_connected = client_connected;
	opts.cbs.sock_disconnected = client_disconnected;
	opts.cbs.sock_can_read = client_can_read;
	opts.cbs.sock_can_write = client_can_write;

	/* create socket server */
	srv = vtm_socket_stream_srv_new();
	if (!srv) {
		vtm_err_print();
		goto end;
	}

	/* run socket server */
	rc = vtm_socket_stream_srv_run(srv, &opts);
	if (rc != VTM_OK)
		vtm_err_print();

end:
	/* free resources */
	vtm_socket_stream_srv_free(srv);

	/* shutdown network module */
	vtm_module_network_end();

	return 0;
}
