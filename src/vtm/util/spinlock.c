/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "spinlock.h"

/* busy wait */
#if defined(__GNUC__) || defined(__clang__)
	#if defined(__i386__) || defined(__x86_64__)
		#define VTM_BUSY_WAIT    __asm__ __volatile__("pause")
	#else
		#define VTM_BUSY_WAIT    __asm__ __volatile__("")
	#endif
#elif defined(_MSC_VER)
	#include <windows.h>
	#define VTM_BUSY_WAIT        YieldProcessor()
#else
	#define VTM_BUSY_WAIT
#endif

void vtm_spinlock_init(struct vtm_spinlock *lock)
{
	lock->locked = 0;
}

void vtm_spinlock_lock(struct vtm_spinlock *lock)
{
	while (VTM_ATOMIC_CAS_INT32(&lock->locked, 0, 1) != 0) {
		VTM_BUSY_WAIT;
	}
}

void vtm_spinlock_unlock(struct vtm_spinlock *lock)
{
	VTM_ATOMIC_ZERO_INT32(&lock->locked);
}
