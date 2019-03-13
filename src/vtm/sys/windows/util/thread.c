/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/util/thread.h>

#include <stdlib.h> /* malloc() */
#include <windows.h>
#include <vtm/core/error.h>
#include <vtm/util/atomic.h>

struct vtm_thread
{
	HANDLE thread;
	DWORD id;
	vtm_thread_func func;
	int result;
	void *arg;
	vtm_atomic_flag running;
};

DWORD WINAPI vtm_thread_function(LPVOID vth)
{
	vtm_thread *th;

	th = vth;
	th->result = th->func(th->arg);
	vtm_atomic_flag_unset(th->running);

	return 0;
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
	th->thread = CreateThread(NULL, 0, vtm_thread_function, th, 0, &th->id);
	if (!th->thread) {
		free(th);
		return NULL;
	}

	return th;
}

void vtm_thread_free(vtm_thread *th)
{
	CloseHandle(th->thread);
	free(th);
}

int vtm_thread_signal(vtm_thread *th, enum vtm_signal_type sig)
{
	return VTM_E_NOT_SUPPORTED;
}

int vtm_thread_cancel(vtm_thread *th)
{
	if (TerminateThread(th->thread, 0) == 0)
		return VTM_ERROR;

	return VTM_OK;
}

int vtm_thread_join(vtm_thread *th)
{
	if (WaitForSingleObject(th->thread, INFINITE) != 0)
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
	return th->id;
}

unsigned long vtm_thread_get_current_id()
{
	return GetCurrentThreadId();
}

void vtm_thread_sleep(unsigned int millis)
{
	Sleep(millis);
}
