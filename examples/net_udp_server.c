/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <stdio.h>
#include <string.h>
#include <vtm/core/error.h>
#include <vtm/net/socket_dgram_server.h>
#include <vtm/util/signal.h>
#include <vtm/util/thread.h>

vtm_socket_dgram_srv *srv;

void stop_server(int psig)
{
	vtm_signal_safe_puts("Stopping server...\n");
	vtm_socket_dgram_srv_stop(srv);
}

void server_ready(vtm_socket_dgram_srv *srv, struct vtm_socket_dgram_srv_opts *opts)
{
	printf("UDP Server listening at: %s:%u\n", opts->addr.host, opts->addr.port);
	fflush(stdout);
}

void worker_init(vtm_socket_dgram_srv *srv, vtm_dataset *wd)
{
	printf("[%lu] Worker started\n", vtm_thread_get_current_id());
}

void worker_end(vtm_socket_dgram_srv *srv, vtm_dataset *wd)
{
	printf("[%lu] Worker finished\n", vtm_thread_get_current_id());
}

void dgram_received(vtm_socket_dgram_srv *srv, vtm_dataset *wd, struct vtm_socket_dgram *dgram)
{
	int rc;
	char addr[VTM_SOCK_ADDR_BUF_LEN];
	unsigned int port;
	enum vtm_socket_family fam;
	long thread_id;

	thread_id = vtm_thread_get_current_id();

	rc = vtm_socket_os_addr_convert(&dgram->saddr, &fam, addr, sizeof(addr), &port);
	if (rc != VTM_OK) {
		fprintf(stderr, "[%lu] Could not get remote addr\n", thread_id);
	}

	printf("[%lu] Received: %.*s (%d bytes) from %s:%d\n",
		thread_id, (int) dgram->buf.len, dgram->buf.data,
		(int) dgram->buf.len, addr, port);

	printf("[%lu] Simulating long running operation\n", thread_id);
	vtm_thread_sleep(3000);
	printf("[%lu] Sending simulated result\n", thread_id);

	vtm_socket_dgram_srv_send(srv, "Pong\n", strlen("Pong\n"), &dgram->saddr);
}

int main(void)
{
	int rc;
	struct vtm_socket_dgram_srv_opts opts;

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
	opts.threads = 2;
	opts.cbs.server_ready = server_ready;
	opts.cbs.worker_init = worker_init;
	opts.cbs.worker_end = worker_end;
	opts.cbs.dgram_recv = dgram_received;

	/* create socket server */
	srv = vtm_socket_dgram_srv_new();
	if (!srv) {
		vtm_err_print();
		goto end;
	}

	/* run socket server */
	rc = vtm_socket_dgram_srv_run(srv, &opts);
	if (rc != VTM_OK)
		vtm_err_print();

end:
	/* free resources */
	vtm_socket_dgram_srv_free(srv);

	/* shutdown network module */
	vtm_module_network_end();

	return 0;
}
