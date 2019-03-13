/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <vtm/core/error.h>
#include <vtm/util/latch.h>
#include <vtm/util/spinlock.h>
#include <vtm/util/thread.h>

#define OPERATIONS    100000
#define THREADS       8

static struct vtm_latch latch;
static struct vtm_spinlock lock;
static int operations;
static int count;

int thread_func(void *arg)
{
	bool running;

	vtm_latch_await(&latch);

	running = true;
	while (running) {
		vtm_spinlock_lock(&lock);

		if (operations-- > 0)
			count++;
		else
			running = false;

		vtm_spinlock_unlock(&lock);
	}

	return VTM_OK;
}


static void test_spinlock(void)
{
	int i;
	vtm_thread *th[THREADS];

	operations = OPERATIONS;
	count = 0;

	vtm_latch_init(&latch, 1);
	vtm_spinlock_init(&lock);

	/* start threads */
	for (i=0; i < THREADS; i++) {
		th[i] = vtm_thread_new(thread_func, NULL);
		VTM_TEST_ASSERT(th[i] != NULL, "spinlock thread started");
	}

	/* signal threads */
	vtm_latch_count(&latch);

	/* wait for threads to end */
	for (i=0; i < THREADS; i++) {
		vtm_thread_join(th[i]);
		vtm_thread_free(th[i]);
	}

	/* check result */
	VTM_TEST_CHECK(count == OPERATIONS, "spinlock result");

	/* release latch */
	vtm_latch_release(&latch);
}

extern void test_vtm_util_spinlock(void)
{
	VTM_TEST_LABEL("spinlock");
	test_spinlock();
}
