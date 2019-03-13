/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include <vtm/util/mutex.h>

#include <stdlib.h> /* malloc() */
#include <windows.h>
#include <psapi.h>
#include <vtm/core/error.h>

struct vtm_mutex
{
	CRITICAL_SECTION mtx;
};

struct vtm_cond
{
	CONDITION_VARIABLE cond;
};

vtm_mutex* vtm_mutex_new(void)
{
	vtm_mutex *mtx;
	
	mtx = malloc(sizeof(vtm_mutex));
	if (!mtx) {
		vtm_err_oom();
		return NULL;
	}
		
	InitializeCriticalSection(&(mtx->mtx));

	return mtx;
}

void vtm_mutex_free(vtm_mutex *mtx)
{
	if (mtx == NULL)
		return;
	
	DeleteCriticalSection(&(mtx->mtx));

	free(mtx);
}

void vtm_mutex_lock(vtm_mutex *mtx)
{
	EnterCriticalSection(&(mtx->mtx));
}

void vtm_mutex_unlock(vtm_mutex *mtx)
{
	LeaveCriticalSection(&(mtx->mtx));
}

vtm_cond* vtm_cond_new(void)
{
	vtm_cond *cond;
	
	cond = malloc(sizeof(vtm_cond));
	if (!cond) {
		vtm_err_oom();
		return NULL;
	}
	
	InitializeConditionVariable(&(cond->cond));

	return cond;
}

void vtm_cond_free(vtm_cond *cond)
{
	free(cond);
}

void vtm_cond_signal(vtm_cond *cond)
{
	WakeConditionVariable(&(cond->cond));
}

void vtm_cond_signal_all(vtm_cond *cond)
{
	WakeAllConditionVariable(&(cond->cond));
}

void vtm_cond_wait(vtm_cond *cond, vtm_mutex *mtx)
{
	SleepConditionVariableCS(&(cond->cond), &(mtx->mtx), INFINITE);
}
