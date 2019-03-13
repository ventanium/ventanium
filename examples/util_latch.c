/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <stdio.h>
#include <vtm/core/error.h>
#include <vtm/core/macros.h>
#include <vtm/util/thread.h>
#include <vtm/util/latch.h>

struct vtm_latch start;
struct vtm_latch finished;

int thread_func(void *arg)
{
	printf("Thread started and now waiting for start signal\n");

	/* wait for start signal */
	vtm_latch_await(&start);

	/* sleep */
	vtm_thread_sleep(2000);
	printf("Thread finished sleeping\n");

	/* signal that this thread has reached the latch */
	vtm_latch_count(&finished);

	printf("Thread finished\n");

	return VTM_OK;
}

int main(void)
{
	vtm_thread *th[4];
	unsigned int i;

	vtm_latch_init(&start, 1);
	vtm_latch_init(&finished, VTM_ARRAY_LEN(th));

	for (i=0; i < VTM_ARRAY_LEN(th); i++) {
		th[i] = vtm_thread_new(thread_func, NULL);
		if (!th[i]) {
			vtm_err_print();
			goto clean;
		}
	}

	/* signal the threads to begin */
	vtm_latch_count(&start);

	/* wait for threads to finish */
	vtm_latch_await(&finished);
	printf("All threads finished\n");

	/* free threads */
	for (i=0; i < VTM_ARRAY_LEN(th); i++) {
		vtm_thread_join(th[i]);
		vtm_thread_free(th[i]);
	}

clean:
	/* free latches */
	vtm_latch_release(&start);
	vtm_latch_release(&finished);

	return 0;
}
