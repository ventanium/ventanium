/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "latch.h"

#include <vtm/core/error.h>

int vtm_latch_init(struct vtm_latch *latch, unsigned int count)
{
	if (count == 0)
		return VTM_E_INVALID_ARG;

	latch->mtx = vtm_mutex_new();
	if (!latch->mtx)
		return vtm_err_get_code();

	latch->cond = vtm_cond_new();
	if (!latch->cond)
		return vtm_err_get_code();

	latch->count = count;

	return VTM_OK;
}

void vtm_latch_release(struct vtm_latch *latch)
{
	if (!latch)
		return;

	vtm_cond_free(latch->cond);
	vtm_mutex_free(latch->mtx);
}

void vtm_latch_count(struct vtm_latch *latch)
{
	vtm_mutex_lock(latch->mtx);
	VTM_ASSERT(latch->count > 0);
	latch->count--;
	if (latch->count == 0)
		vtm_cond_signal_all(latch->cond);
	vtm_mutex_unlock(latch->mtx);
}

void vtm_latch_await(struct vtm_latch *latch)
{
	vtm_mutex_lock(latch->mtx);
	while (latch->count > 0)
		vtm_cond_wait(latch->cond, latch->mtx);
	vtm_mutex_unlock(latch->mtx);
}
