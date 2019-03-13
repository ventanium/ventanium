/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <time.h>
#include <vtm/core/error.h>
#include <vtm/net/nm/nm_dgram_client.h>

int main(void)
{
	int rc;
	vtm_nm_dgram_client *cl;
	struct vtm_socket_addr addr;
	struct vtm_socket_saddr src, dst;
	vtm_dataset *msg;
	uint32_t a, b, c;

	/* init network module */
	rc = vtm_module_network_init();
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* create IPv4 UDP client */
	cl = vtm_nm_dgram_client_new(VTM_SOCK_FAM_IN4);
	if (!cl) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* prepare message */
	msg = vtm_dataset_new();
	if (!msg) {
		vtm_err_print();
		goto end;
	}

	srand(time(NULL));
	a = rand() % 500;
	b = rand() % 500;
	vtm_dataset_set_uint32(msg, "A", a);
	vtm_dataset_set_uint32(msg, "B", b);

	/* prepare destination addr */
	addr.family = VTM_SOCK_FAM_IN4;
	addr.host = "127.0.0.1";
	addr.port = 4000;

	rc = vtm_socket_os_addr_build(&dst, &addr);
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end;
	}

	/* send message */
	rc = vtm_nm_dgram_client_send(cl, msg, &dst);
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end;
	}

	/* prepare msg for reuse */
	vtm_dataset_clear(msg);

	/* try to receive answer */
	vtm_nm_dgram_client_set_opt(cl, VTM_NM_DGRAM_CL_OPT_RECV_TIMEOUT,
		(unsigned long[]) {1000}, sizeof(unsigned long));
	rc = vtm_nm_dgram_client_recv(cl, msg, &src);
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end;
	}

	/* display received message */
	c = vtm_dataset_get_uint32(msg, "C");
	printf("Sum of %" PRIu32 " and %" PRIu32 " is %" PRIu32 "\n", a, b, c);

end:
	/* free resources */
	vtm_dataset_free(msg);
	vtm_nm_dgram_client_free(cl);

	/* shutdown network module */
	vtm_module_network_end();

	return 0;
}
