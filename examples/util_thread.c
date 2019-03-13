/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <stdio.h>
#include <vtm/core/error.h>
#include <vtm/util/thread.h>

int thread_func(void *arg)
{
	int i, count;

	/* we know that arg points to an integer */
	count = *(int*) arg;

	printf("Thread started\n");

	for (i=0; i < count; i++) {
		printf("Thread says hello %d\n", i+1);
		vtm_thread_sleep(500);
	}

	printf ("Thread finished\n");

	return VTM_OK;
}

int main(void)
{
	vtm_thread *th;
	int val;

	val = 10;

	/*
	 * Create new thread which runs the function 'thread_func()'.
	 * A pointer to the integer variable 'val' is delivered as input argument
	 * to the thread function.
	 */
	th = vtm_thread_new(thread_func, &val);
	if (!th) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* let main thread sleep for 3 seconds */
	printf("Main thread sleeps now for 3 seconds\n");
	vtm_thread_sleep(3000);

	/* wait for thread to finish */
	printf("Main thread is awake and waits for thread to finish\n");
	vtm_thread_join(th);

	/* free resources */
	vtm_thread_free(th);

	return 0;
}
