/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/core/error.h>
#include <vtm/util/mutex.h>
#include <vtm/util/thread.h>

vtm_mutex *mtx;
int items;
int count[2];

int thread_func(void *arg)
{
	bool running;

	running = true;
	while (running) {
		/* aquire exclusive access */
		vtm_mutex_lock(mtx);

		if (items > 0) {
			/* consume available item */
			items--;
			(*((int*) arg))++;
		}
		else {
			/* end loop */
			running = false;
		}

		/* leave critical section */
		vtm_mutex_unlock(mtx);
	}

	return VTM_OK;
}

int main(void)
{
	vtm_thread *th1, *th2;

	/* create a mutex which is used to synchronize the two threads */
	mtx = vtm_mutex_new();
	if (!mtx) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* prepare items and counters */
	items = 10000;
	count[0] = 0;
	count[1] = 0;

	/* start two threads */
	th1 = vtm_thread_new(thread_func, &count[0]);
	th2 = vtm_thread_new(thread_func, &count[1]);

	/* wait for threads to finish */
	if (th1)
		vtm_thread_join(th1);
	if (th2)
		vtm_thread_join(th2);

	/* display results */
	printf("Thread1 consumed %d items\n", count[0]);
	printf("Thread2 consumed %d items\n", count[1]);

	/* free resources */
	vtm_thread_free(th1);
	vtm_thread_free(th2);
	vtm_mutex_free(mtx);

	return 0;
}
