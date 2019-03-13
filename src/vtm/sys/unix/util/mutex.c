/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include <vtm/util/mutex.h>

#include <stdlib.h> /* malloc() */
#include <pthread.h>
#include <vtm/core/error.h>

struct vtm_mutex
{
	pthread_mutex_t mtx;
};

struct vtm_cond
{
	pthread_cond_t cond;
};

vtm_mutex* vtm_mutex_new(void)
{
	vtm_mutex *mtx;
	pthread_mutexattr_t attr;

	mtx = malloc(sizeof(vtm_mutex));
	if (!mtx) {
		vtm_err_oom();
		return NULL;
	}

	if (pthread_mutexattr_init(&attr) != 0) {
		free(mtx);
		vtm_err_set(VTM_E_MALLOC);
		return NULL;
	}

	if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
		VTM_ABORT_FATAL();

	if (pthread_mutex_init(&mtx->mtx, &attr) != 0) {
		free(mtx);
		mtx = NULL;
		vtm_err_set(VTM_E_MALLOC);
	}

	pthread_mutexattr_destroy(&attr);

	return mtx;
}

void vtm_mutex_free(vtm_mutex *mtx)
{
	if (mtx == NULL)
		return;

	if (pthread_mutex_destroy(&mtx->mtx) != 0)
		VTM_ABORT_FATAL();

	free(mtx);
}

void vtm_mutex_lock(vtm_mutex *mtx)
{
	if (pthread_mutex_lock(&mtx->mtx) != 0)
		VTM_ABORT_FATAL();
}

void vtm_mutex_unlock(vtm_mutex *mtx)
{
	if (pthread_mutex_unlock(&mtx->mtx) != 0)
		VTM_ABORT_FATAL();
}

vtm_cond* vtm_cond_new(void)
{
	vtm_cond *cond;

	cond = malloc(sizeof(vtm_cond));
	if (!cond) {
		vtm_err_oom();
		return NULL;
	}

	if (pthread_cond_init(&cond->cond, NULL) != 0) {
		free(cond);
		return NULL;
	}

	return cond;
}

void vtm_cond_free(vtm_cond *cond)
{
	if (cond == NULL)
		return;

	if (pthread_cond_destroy(&cond->cond) != 0)
		VTM_ABORT_FATAL();

	free(cond);
}

void vtm_cond_signal(vtm_cond *cond)
{
	if (pthread_cond_signal(&cond->cond) != 0)
		VTM_ABORT_FATAL();
}

void vtm_cond_signal_all(vtm_cond *cond)
{
	if (pthread_cond_broadcast(&cond->cond) != 0)
		VTM_ABORT_FATAL();
}

void vtm_cond_wait(vtm_cond *cond, vtm_mutex *mtx)
{
	if (pthread_cond_wait(&cond->cond, &mtx->mtx) != 0)
		VTM_ABORT_FATAL();
}
