/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <string.h>
#include <vtm/core/error.h>
#include <vtm/net/nm/nm_dgram_server.h>
#include <vtm/util/signal.h>
#include <vtm/util/thread.h>

vtm_nm_dgram_srv *srv;

void stop_server(int psig)
{
	vtm_signal_safe_puts("Stopping server...\n");
	vtm_nm_dgram_srv_stop(srv);
}

void server_ready(vtm_nm_dgram_srv *srv, struct vtm_nm_dgram_srv_opts *opts)
{
	printf("NM UDP Server listening at: %s:%u\n", opts->addr.host, opts->addr.port);
	fflush(stdout);
}

void worker_init(vtm_nm_dgram_srv *srv, vtm_dataset *wd)
{
	printf("[%lu] Worker started\n", vtm_thread_get_current_id());
}

void worker_end(vtm_nm_dgram_srv *srv, vtm_dataset *wd)
{
	printf("[%lu] Worker finished\n", vtm_thread_get_current_id());
}

void msg_recv(vtm_nm_dgram_srv *srv, vtm_dataset *wd, vtm_dataset *msg, const struct vtm_socket_saddr *saddr)
{
	uint32_t a, b, c;

	/* calculate request */
	a = vtm_dataset_get_uint32(msg, "A");
	b = vtm_dataset_get_uint32(msg, "B");
	c = a + b;

	/* display request */
	printf("[%lu] %" PRIu32 " + %" PRIu32 " = %" PRIu32 "\n",
		vtm_thread_get_current_id(), a, b, c);

	/* send response */
	vtm_dataset_clear(msg);
	vtm_dataset_set_uint32(msg, "C", c);
	vtm_nm_dgram_srv_send(srv, msg, saddr);
}

int main(void)
{
	int rc;
	struct vtm_nm_dgram_srv_opts opts;

	/* register signal handler */
	vtm_signal_set_handler(VTM_SIG_INT, stop_server);

	/* init network module */
	rc = vtm_module_network_init();
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* create dgram server */
	srv = vtm_nm_dgram_srv_new();
	if (!srv) {
		vtm_err_print();
		goto end;
	}

	/* prepare options */
	memset(&opts, 0, sizeof(opts));
	opts.addr.family = VTM_SOCK_FAM_IN4;
	opts.addr.host = "127.0.0.1";
	opts.addr.port = 4000;
	opts.threads = 4;
	opts.cbs.server_ready = server_ready;
	opts.cbs.worker_init = worker_init;
	opts.cbs.worker_end = worker_end;
	opts.cbs.msg_recv = msg_recv;

	/* run server */
	rc = vtm_nm_dgram_srv_run(srv, &opts);
	if (rc != VTM_OK)
		vtm_err_print();

	/* free resources */
	vtm_nm_dgram_srv_free(srv);

end:
	/* shutdown network module */
	vtm_module_network_end();

	return 0;
}
