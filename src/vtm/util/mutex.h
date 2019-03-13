/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file mutex.h
 *
 * @brief Mutex and condition variables
 */

#ifndef VTM_UTIL_MUTEX_H_
#define VTM_UTIL_MUTEX_H_

#include <vtm/core/api.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtm_mutex vtm_mutex;
typedef struct vtm_cond vtm_cond;

/**
 * Creates a new mutex.
 *
 * @return a mutex handle
 * @return NULL if an error occured
 */
VTM_API vtm_mutex* vtm_mutex_new(void);

/**
 * Releases the mutex handle and all allocates resources.
 *
 * @param mtx the mutex which should be released
 */
VTM_API void vtm_mutex_free(vtm_mutex *mtx);

/**
 * Locks the given mutex.
 *
 * This call may block when the mutex is already locked by
 * another thread.
 *
 * @param mtx the mutex which should be locked
 */
VTM_API void vtm_mutex_lock(vtm_mutex *mtx);

/**
 * Unlocks the given mutex.
 *
 * @param mtx the mutex which should be unlocked
 */
VTM_API void vtm_mutex_unlock(vtm_mutex *mtx);

/**
 * Creates a new condition variable.
 *
 * @return a condition variable handle
 * @return NULL if an error occured
 */
VTM_API vtm_cond* vtm_cond_new(void);

/**
 * Releases the condition variable and all allocates resources.
 *
 * @param cond the condition variable which should be released
 */
VTM_API void vtm_cond_free(vtm_cond *cond);

/**
 * Wait until given condition variable is signaled.
 *
 * The mutex must be already locked by the current thread when
 * this method is called. The mutex is unlocked while waiting and
 * is re-locked when the call returns.
 *
 * Due to spurious wakeups on some platforms, the condition should
 * always be checked again after this call has returned.
 *
 * @param cond the condition variable on which the wait is performed
 * @param mtx the mutex which is used
 */
VTM_API void vtm_cond_wait(vtm_cond *cond, vtm_mutex *mtx);

/**
 * Wakes up one thread that is waiting on this condition variable.
 *
 * @param cond the condition variable which should be signaled
 */
VTM_API void vtm_cond_signal(vtm_cond *cond);

/**
 * Wakes up all threads that are waiting on this condition.
 *
 * @param cond the condition variable which should be signaled
 */
VTM_API void vtm_cond_signal_all(vtm_cond *cond);

#ifdef __cplusplus
}
#endif

#endif /* VTM_UTIL_MUTEX_H_ */
