/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file spinlock.h
 *
 * @brief Simple spinlock
 */

#ifndef VTM_UTIL_SPINLOCK_H_
#define VTM_UTIL_SPINLOCK_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/util/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vtm_spinlock
{
	VTM_ATOMIC_INT32_TYPE locked;
};

/**
 * Initializes the spinlock.
 *
 * @param lock the spinlock that should be initialized
 */
VTM_API void vtm_spinlock_init(struct vtm_spinlock *lock);

/**
 * Locks the spinlock.
 *
 * @param lock the spinlock that should be locked
 */
VTM_API void vtm_spinlock_lock(struct vtm_spinlock *lock);

/**
 * Unlocks the spinlock.
 *
 * @param lock the spinlock that should be unlocked
 */
VTM_API void vtm_spinlock_unlock(struct vtm_spinlock *lock);

#ifdef __cplusplus
}
#endif

#endif /* VTM_UTIL_SPINLOCK_H_ */
