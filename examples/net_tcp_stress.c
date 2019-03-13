/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <string.h>
#include <vtm/core/error.h>
#include <vtm/core/macros.h>
#include <vtm/net/socket.h>
#include <vtm/util/thread.h>
#include <vtm/util/signal.h>

#define HOST      "127.0.0.1"
#define PORT      5000
#define THREADS   16

volatile bool loop;

void stop(int psig)
{
	loop = false;
}

void send_data(void)
{
	int rc, i;
	vtm_socket *sock;
	size_t written;

	sock = vtm_socket_new(VTM_SOCK_FAM_IN4, VTM_SOCK_TYPE_STREAM);
	if (!sock) {
		vtm_err_print();
		return;
	}

	rc = vtm_socket_connect(sock, HOST, PORT);
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end;
	}

	for (i=0; i < 100; i++) {
		rc = vtm_socket_write(sock, HOST, sizeof(HOST), &written);
		if (rc != VTM_OK)
			break;
	}
	
	vtm_socket_close(sock);

end:
	vtm_socket_free(sock);
}

int thread_func(void *arg)
{
	while(loop)
		send_data();

	return 0;
}

int main(void)
{
	int rc;
	vtm_thread *th[THREADS];
	size_t i;

	/* register signal handler */
	vtm_signal_set_handler(VTM_SIG_INT, stop);

	/* init network module */
	rc = vtm_module_network_init();
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* start threads */
	memset(&th, 0, sizeof(th));
	loop = true;
	for (i=0; i < VTM_ARRAY_LEN(th); i++) {
		th[i] = vtm_thread_new(thread_func, NULL);
		if (!th[i]) {
			vtm_err_print();
			loop = false;
			break;
		}
	}

	/* free threads */
	for (i=0; i < VTM_ARRAY_LEN(th); i++) {
		vtm_thread_join(th[i]);
		vtm_thread_free(th[i]);
	}

	/* shutdown network module */
	vtm_module_network_end();

	return 0;
}
