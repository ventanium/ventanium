/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file latch.h
 *
 * @brief Thread synchronisation helper
 */

#ifndef VTM_UTIL_LATCH_H_
#define VTM_UTIL_LATCH_H_

#include <vtm/core/api.h>
#include <vtm/util/mutex.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vtm_latch
{
	unsigned int count;
	vtm_mutex *mtx;
	vtm_cond *cond;
};

/**
 * Initializes a new latch.
 *
 * @param latch pointer to the latch, can be on stack or heap
 * @param count initial counter of the latch
 * @return VTM_OK when the latch was successfully initialized
 * @return VTM_ERROR when the latch could not be initialized
 */
VTM_API int vtm_latch_init(struct vtm_latch *latch, unsigned int count);

/**
 * Releases all resources of the given latch
 *
 * @param latch the latch which should be released
 */
VTM_API void vtm_latch_release(struct vtm_latch *latch);

/**
 * Decrements the latch counter by one.
 *
 * If the counter reaches zero, all threads waiting in
 * vtm_latch_await() are released and continue their execution.
 *
 * @param latch the target latch
 */
VTM_API void vtm_latch_count(struct vtm_latch *latch);

/**
 * Waits until the latch counter reaches zero.
 *
 * @param latch the target latch
 */
VTM_API void vtm_latch_await(struct vtm_latch *latch);

#ifdef __cplusplus
}
#endif

#endif /* VTM_UTIL_LATCH_H_ */
