/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <string.h>
#include <vtm/core/error.h>
#include <vtm/crypto/crypto.h>
#include <vtm/net/http/ws_client.h>

int main(void)
{
	int rc;
	vtm_ws_client *cl;
	struct vtm_ws_msg msg;

	/* init modules */
	rc = vtm_module_crypto_init();
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	rc = vtm_module_network_init();
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end_crypto;
	}

	/* create new ws client */
	cl = vtm_ws_client_new();
	if (!cl) {
		vtm_err_print();
		goto end_net;
	}

	/* since TLS certificate verification is not working yet, disable it */
	vtm_ws_client_set_opt(cl, VTM_WS_CL_OPT_NO_CERT_CHECK, (bool[]) {true}, sizeof(bool));

	/* connect */
	rc = vtm_ws_client_connect(cl, VTM_SOCK_FAM_IN4, "http://127.0.0.1:5000/echo");
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end;
	}

	/* send message */
	rc = vtm_ws_client_send(cl, VTM_WS_MSG_TEXT, "Hello", strlen("Hello"));
	if (rc != VTM_OK)
		goto end;

	/* receive reponse */
	rc = vtm_ws_client_recv(cl, &msg);
	if (rc != VTM_OK)
		goto end;

	/* display response */
	printf("Type=%u, Msg=%.*s\n", msg.type, (int) msg.len, (char*) msg.data);
	vtm_ws_msg_release(&msg);
	
end:
	/* free http client */
	vtm_ws_client_free(cl);

end_net:
	vtm_module_network_end();

end_crypto:
	vtm_module_crypto_end();

	return 0;
}
