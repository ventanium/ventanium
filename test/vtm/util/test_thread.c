/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <vtm/core/error.h>
#include <vtm/util/thread.h>
#include <vtm/util/time.h>

#define SLEEP_TIME 1500

static int test_thread_func(void *arg)
{
	vtm_thread_sleep(SLEEP_TIME);
	return VTM_OK;
}

static void test_thread(void)
{
	uint64_t start = vtm_time_current_millis();
	vtm_thread *th = vtm_thread_new(test_thread_func, NULL);
	VTM_TEST_CHECK(th != NULL, "thread create");

	int rc = vtm_thread_join(th);
	VTM_TEST_CHECK(rc == VTM_OK, "thread join");

	uint64_t end = vtm_time_current_millis();
	VTM_TEST_CHECK(end - start >= SLEEP_TIME, "thread sleep check");

	rc = vtm_thread_get_result(th);
	VTM_TEST_CHECK(rc == VTM_OK, "thread result check");

	vtm_thread_free(th);
	VTM_TEST_PASSED("thread free");
}

extern void test_vtm_util_thread(void)
{
	VTM_TEST_LABEL("thread");
	test_thread();
}
