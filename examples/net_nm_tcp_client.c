/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <time.h>
#include <vtm/core/error.h>
#include <vtm/net/nm/nm_stream_client.h>

int main(void)
{
	int rc, i;
	vtm_nm_stream_client *cl;
	struct vtm_nm_stream_client_opts opts;
	vtm_dataset *msg;
	uint32_t a, b, c, d;

	msg = NULL;

	/* init network module */
	rc = vtm_module_network_init();
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* create TCP client */
	cl = vtm_nm_stream_client_new();
	if (!cl) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* set timeout */
	vtm_nm_stream_client_set_opt(cl, VTM_NM_STREAM_CL_OPT_RECV_TIMEOUT,
		(unsigned long[]) {1000}, sizeof(unsigned long));

	/* prepare destination */
	opts.addr.family = VTM_SOCK_FAM_IN4;
	opts.addr.host = "127.0.0.1";
	opts.addr.port = 4000;
	opts.tls.enabled = false;

	/* connect */
	rc = vtm_nm_stream_client_connect(cl, &opts);
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end;
	}

	/* prepare message */
	msg = vtm_dataset_new();
	if (!msg) {
		vtm_err_print();
		goto end;
	}

	/* init random number generator */
	srand(time(NULL));

	/* send messages */
	for (i=0; i < 3; i++) {
		/* prepare msg for reuse */
		vtm_dataset_clear(msg);

		a = rand() % 500;
		b = rand() % 500;
		vtm_dataset_set_uint32(msg, "A", a);
		vtm_dataset_set_uint32(msg, "B", b);

		/* send message */
		rc = vtm_nm_stream_client_send(cl, msg);
		if (rc != VTM_OK) {
			vtm_err_print();
			goto end;
		}

		/* prepare msg for reuse */
		vtm_dataset_clear(msg);

		/* try to receive answer */
		rc = vtm_nm_stream_client_recv(cl, msg);
		if (rc != VTM_OK) {
			vtm_err_print();
			goto end;
		}

		/* display received message */
		c = vtm_dataset_get_uint32(msg, "C");
		d = vtm_dataset_get_uint32(msg, "D");
		printf("Sum of %" PRIu32 " and %" PRIu32 " is %" PRIu32 "\n", a, b, c);
		printf("Total sum is %" PRIu32 "\n", d);
	}

end:
	/* free resources */
	vtm_dataset_free(msg);
	vtm_nm_stream_client_free(cl);

	/* shutdown network module */
	vtm_module_network_end();

	return 0;
}
