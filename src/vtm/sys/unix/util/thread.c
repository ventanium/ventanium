/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include <vtm/util/thread.h>

#include <stdlib.h> /* malloc() */
#include <signal.h>
#include <pthread.h>
#include <time.h> /* nanosleep() */

#include <vtm/core/error.h>
#include <vtm/util/atomic.h>

struct vtm_thread
{
	pthread_t thread;
	vtm_thread_func func;
	void *arg;
	int result;
	vtm_atomic_flag running;
};

static void* vtm_thread_function(void *vth)
{
	vtm_thread *th;

	th = vth;
	th->result = th->func(th->arg);
	vtm_atomic_flag_unset(th->running);

	return NULL;
}

vtm_thread* vtm_thread_new(vtm_thread_func func, void *arg)
{
	vtm_thread *th;

	th = malloc(sizeof(vtm_thread));
	if (!th) {
		vtm_err_oom();
		return NULL;
	}

	th->func = func;
	th->arg = arg;
	th->result = VTM_OK;

	vtm_atomic_flag_init(th->running, true);
	if (pthread_create(&th->thread, NULL, vtm_thread_function, th) != 0) {
		free(th);
		return NULL;
	}

	return th;
}

void vtm_thread_free(vtm_thread *th)
{
	free(th);
}

int vtm_thread_signal(vtm_thread *th, enum vtm_signal_type sig)
{
	int rc;
	int psig;

	rc = vtm_signal_convert_to_os(sig, &psig);
	if (rc != VTM_OK)
		return rc;

	if (pthread_kill(th->thread, psig) != 0)
		return VTM_ERROR;

	return VTM_OK;
}

int vtm_thread_cancel(vtm_thread *th)
{
	if (pthread_cancel(th->thread) != 0)
		return VTM_ERROR;
	return VTM_OK;
}

int vtm_thread_join(vtm_thread *th)
{
	if (pthread_join(th->thread, NULL) != 0)
		return VTM_ERROR;
	return VTM_OK;
}

bool vtm_thread_running(vtm_thread *th)
{
	return vtm_atomic_flag_isset(th->running);
}

int vtm_thread_get_result(vtm_thread *th)
{
	return th->result;
}

unsigned long vtm_thread_get_id(vtm_thread *th)
{
	return (unsigned long) th->thread;
}

unsigned long vtm_thread_get_current_id()
{
	return (unsigned long) pthread_self();
}

void vtm_thread_sleep(unsigned int millis)
{
	struct timespec ts;

	ts.tv_sec = millis / 1000;
	ts.tv_nsec = (millis % 1000) * 1000 * 1000;

	nanosleep(&ts, NULL);
}
