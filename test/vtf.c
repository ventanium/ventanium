/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "vtf.h"

#include <stdlib.h> /* abort() */
#include <stdio.h> /* printf(), fprintf() */

static unsigned int opt_print_passed = 0;

static unsigned int test_total;
static unsigned int test_failed;

static const char *test_current_module;
static const char *test_current_label;
static const char *test_current_file;

void vtm_test_set_option(enum vtm_test_opt opt, int val)
{
	switch (opt) {
		case VTM_TEST_OPT_PRINT_PASSED:
			opt_print_passed = val;
			break;

		default:
			fprintf(stderr, "Option not supported: %d\n", opt);
			break;
	}
}

void vtm_test_begin(void)
{
	printf("Test started..\n");
	fflush(stdout);

	test_total = 0;
	test_failed = 0;

	test_current_module = NULL;
	test_current_label = NULL;
	test_current_file = NULL;
}

void vtm_test_set_module(const char *name)
{
	test_current_module = name;
	printf("-- Module: %s --\n", name);
	fflush(stdout);
}

void vtm_test_set_label(const char *name, const char *file)
{
	printf("Testing label: %s (%s)\n", name, file);
	fflush(stdout);
}

void vtm_test_run(vtm_test_func fn)
{
	fn();
}

void vtm_test_summary(void)
{
	unsigned int test_passed = test_total-test_failed;
	float rate = test_total ? (test_passed / (float) test_total) * 100 : 100;

	printf("-- Summary --\n");
	printf("Total tests:   %6d\n", test_total);
	printf("Tests passed:  %6d\n", test_passed);
	printf("Tests failed:  %6d\n", test_failed);
	printf("Test success rate is %.2f %%\n", rate);
	fflush(stdout);
}

void vtm_test_end(void)
{
	printf("Test finished!\n");
	fflush(stdout);
}

void vtm_test_check(int result, const char *desc, const char *file, unsigned int line)
{
	test_total++;
	if (result)
		vtm_test_passed(desc, file, line);
	else
		vtm_test_failed(desc, file, line);
}

void vtm_test_passed(const char *desc, const char *file, unsigned int line)
{
	if (!opt_print_passed)
		return;

	printf("TEST PASSED: %s, %s:%d\n", desc, file, line);
	fflush(stdout);
}

void vtm_test_failed(const char *desc, const char *file, unsigned int line)
{
	test_failed++;
	printf("TEST FAILED: %s, %s:%d\n", desc, file, line);
	fflush(stdout);
}

void vtm_test_assert(int result, const char *desc, const char *file, unsigned int line)
{
	test_total++;
	if (result)
		return;

	test_failed++;
	vtm_test_abort(desc, file, line);
}

void vtm_test_abort(const char *desc, const char *file, unsigned int line)
{
	fprintf(stderr, "TEST ABORT: %s, %s:%d\n", desc, file, line);
	abort();
}
