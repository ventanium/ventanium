/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "vtf.h"

#include <stdlib.h> /* exit codes */
#include <stdio.h> /* printf(), fprintf() */

/* core */
extern void test_vtm_core_blob(void);
extern void test_vtm_core_elem(void);
extern void test_vtm_core_string(void);
extern void test_vtm_core_list(void);
extern void test_vtm_core_map(void);
extern void test_vtm_core_variant(void);
extern void test_vtm_core_dataset(void);
extern void test_vtm_core_format(void);

void test_core(void)
{
	vtm_test_set_module("core");
	vtm_test_run(test_vtm_core_blob);
	vtm_test_run(test_vtm_core_format);
	vtm_test_run(test_vtm_core_string);
	vtm_test_run(test_vtm_core_list);
	vtm_test_run(test_vtm_core_map);
	vtm_test_run(test_vtm_core_elem);
	vtm_test_run(test_vtm_core_variant);
	vtm_test_run(test_vtm_core_dataset);
}

/* crypt */
#ifdef VTM_MODULE_CRYPTO
extern void test_vtm_crypto_hash(void);
#endif

void test_crypt(void)
{
	vtm_test_set_module("crypto");
#ifdef VTM_MODULE_CRYPTO
	vtm_test_run(test_vtm_crypto_hash);
#endif
}

/* fs */
extern void test_vtm_fs_config(void);
extern void test_vtm_fs_file(void);
extern void test_vtm_fs_log(void);

void test_fs(void)
{
	vtm_test_set_module("fs");
	vtm_test_run(test_vtm_fs_config);
	vtm_test_run(test_vtm_fs_file);
	vtm_test_run(test_vtm_fs_log);
}

/* net */
extern void test_vtm_net_http_server(void);
extern void test_vtm_net_nm_dgram(void);
extern void test_vtm_net_nm_stream(void);
extern void test_vtm_net_nm_stream_mt(void);
extern void test_vtm_net_url(void);

void test_net(void)
{
	vtm_test_set_module("net");
	vtm_test_run(test_vtm_net_url);
	vtm_test_run(test_vtm_net_nm_dgram);
	vtm_test_run(test_vtm_net_nm_stream);
	vtm_test_run(test_vtm_net_nm_stream_mt);
	vtm_test_run(test_vtm_net_http_server);
}

/* sql */
#ifdef VTM_MODULE_MYSQL
extern void test_vtm_sql_mysql(void);
#endif

#ifdef VTM_MODULE_SQLITE
extern void test_vtm_sql_sqlite(void);
#endif

void test_sql(void)
{
	vtm_test_set_module("sql");

#ifdef VTM_MODULE_MYSQL
	vtm_test_run(test_vtm_sql_mysql);
#endif

#ifdef VTM_MODULE_SQLITE
	vtm_test_run(test_vtm_sql_sqlite);
#endif
}

/* util */
extern void test_vtm_util_base64(void);
extern void test_vtm_util_thread(void);
extern void test_vtm_util_serialization(void);
extern void test_vtm_util_spinlock(void);

void test_util(void)
{
	vtm_test_set_module("util");
	vtm_test_run(test_vtm_util_base64);
	vtm_test_run(test_vtm_util_thread);
	vtm_test_run(test_vtm_util_serialization);
	vtm_test_run(test_vtm_util_spinlock);
}

void test_suite(void)
{
	vtm_test_begin();

	test_core();
	test_util();
	test_fs();
	test_crypt();
	test_net();
	test_sql();

	vtm_test_summary();
	vtm_test_end();
}

void print_usage(char *prog)
{
	char usage[] = "Usage: %s [option]\n"
		 "Possible options:\n"
		 " -p : print passed test info\n"
		 " -h : print usage help\n";
	printf(usage, prog);
}

int main(int argc, char **argv)
{
	char *prog = argv[0];
	while (--argc > 0 && (*++argv)[0] == '-') {
		int c;
		while ((c = *++argv[0])) {
			switch (c) {
				case 'p':
					vtm_test_set_option(VTM_TEST_OPT_PRINT_PASSED, 1);
					break;

				case 'h':
					print_usage(prog);
					return EXIT_SUCCESS;

				default:
					fprintf(stderr, "unknown argument: %c\n", c);
					print_usage(prog);
					return EXIT_FAILURE;
			}
		}
	}

	test_suite();
	return EXIT_SUCCESS;
}
