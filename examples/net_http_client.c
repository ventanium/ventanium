/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/core/error.h>
#include <vtm/crypto/crypto.h>
#include <vtm/net/http/http_client.h>

int main(void)
{
	int rc;
	vtm_http_client *cl;
	struct vtm_http_client_req req;
	struct vtm_http_client_res res;
	vtm_list *entries;
	struct vtm_dataset_entry *entry;
	size_t i, n;

	/* init modules */
	rc = vtm_module_crypto_init();
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	rc = vtm_module_network_init();
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* create new http client */
	cl = vtm_http_client_new();
	if (!cl) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* since TLS certificate verification is not working yet, disable it */
	vtm_http_client_set_opt(cl, VTM_HTTP_CL_OPT_NO_CERT_CHECK,
		                    (bool[]) {true}, sizeof(bool));

	/* set 5 second timeout */
	vtm_http_client_set_opt(cl, VTM_HTTP_CL_OPT_TIMEOUT,
		                    (unsigned long[]) {5000}, sizeof(unsigned long));

	/* prepare request */
	req.method = VTM_HTTP_METHOD_GET;
	req.version = VTM_HTTP_VER_1_1;
	req.fam = VTM_SOCK_FAM_IN4;
	req.url = "http://127.0.0.1/";
	req.body = NULL;
	req.body_len = 0;

	/* set headers */
	req.headers = vtm_dataset_new();
	if (!req.headers) {
		vtm_err_print();
		goto end;
	}

	vtm_dataset_set_string(req.headers, "Some-Header", "Value");

	/* request */
	rc = vtm_http_client_request(cl, &req, &res);
	if (rc != VTM_OK) {
		vtm_err_print();
		goto end;
	}

	/* display response */
	printf("Response: %s %d %s\n", vtm_http_get_version_string(res.version),
		res.status_code, res.status_msg);

	entries = vtm_dataset_entryset(res.headers);
	if (entries) {
		n = vtm_list_size(entries);
		for (i=0; i < n; i++) {
			entry = vtm_list_get_pointer(entries, i);
			printf("%s: %s\n", entry->name, vtm_variant_as_str(entry->var));
		}
		vtm_list_free(entries);
	}

	printf("Body: %.*s\n", (int) res.body_len, (char*) res.body);

	/* release response memory */
	vtm_http_client_res_release(&res);

end:
	/* free request headers */
	vtm_dataset_free(req.headers);

	/* free http client */
	vtm_http_client_free(cl);

	/* end modules */
	vtm_module_network_end();
	vtm_module_crypto_end();

	return 0;
}
