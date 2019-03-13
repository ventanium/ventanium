/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/core/error.h>
#include <vtm/util/mutex.h>
#include <vtm/util/thread.h>

vtm_mutex *mtx;
vtm_cond *cond_available;
int available;
int consumed;
volatile bool running;

void produce(void)
{
	int i;

	for (i=0; i < 10; i++) {
		vtm_mutex_lock(mtx);

		/* produce one item */
		available++;
		printf("Produced one item\n");

		/* wake up at least one thread who is waiting on this condition */
		vtm_cond_signal(cond_available);

		vtm_mutex_unlock(mtx);

		/* wait some time before next item is produced */
		vtm_thread_sleep(100);
	}

	printf ("Producer finished\n");
}

int thread_func(void *arg)
{
	while (running) {
		vtm_mutex_lock(mtx);
		while (available == 0) {
			/* check if thread should still run */
			if (!running)
				goto unlock;

			/*
			 * If no item is available, let current thread sleep until
			 * condition is signaled. This should always be done in a loop
			 * because of spurious wakeups.
			 */
			vtm_cond_wait(cond_available, mtx);
		}

		/* consume one item */
		available--;
		consumed++;
		printf("Consumed one item\n");

unlock:
		vtm_mutex_unlock(mtx);
	}

	printf("Consumer finished\n");

	return VTM_OK;
}

int main(void)
{
	vtm_thread *th;

	cond_available = NULL;
	th = NULL;

	/* create mutex */
	mtx = vtm_mutex_new();
	if (!mtx) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* create condition variable */
	cond_available = vtm_cond_new();
	if (!cond_available) {
		vtm_err_print();
		goto end;
	}

	/* start consumer thread */
	running = true;
	th = vtm_thread_new(thread_func, NULL);
	if (!th) {
		vtm_err_print();
		goto end;
	}

	/* start producing */
	produce();

	/*
	 * Tell consumer thread to stop.
	 * Since the thread could be blocked in waiting on the condition variable,
	 * we signal the condition after setting the 'running' flag to false.
	 */
	running = false;
	vtm_cond_signal_all(cond_available);

	/* wait for thread to finish */
	vtm_thread_join(th);

	/* display results */
	printf("%d items consumed\n", consumed);

end:
	/* free resources */
	vtm_thread_free(th);
	vtm_cond_free(cond_available);
	vtm_mutex_free(mtx);

	return 0;
}
