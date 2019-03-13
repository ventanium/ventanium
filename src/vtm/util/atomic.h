/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file atomic.h
 *
 * @brief Atomic memory access
 */

#ifndef VTM_UTIL_ATOMIC_H_
#define VTM_UTIL_ATOMIC_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>

#ifdef VTM_SYS_WINDOWS
	#include <windows.h>
#endif

/* memory barrier */
#if defined(__GNUC__) || defined(__clang__)
	#define VTM_MEM_BARRIER    __sync_synchronize
#elif defined(_MSC_VER)
	#define VTM_MEM_BARRIER    MemoryBarrier
#else
	#error VTM_MEM_BARRIER not supported
#endif

/* types */
#if defined(__GNUC__) || defined(__clang__)
	#define VTM_ATOMIC_INT32_TYPE  int32_t
#elif defined(_MSC_VER)
	#define VTM_ATOMIC_INT32_TYPE  volatile LONG
#else
	#error VTM_ATOMIC_INT32_TYPE
#endif

/* atomic add */
#if defined(__GNUC__) || defined(__clang__)
	#define VTM_ATOMIC_ADD_INT32(PTR,VAL)   __sync_add_and_fetch(PTR, VAL)
#elif defined(_MSC_VER)
	#define VTM_ATOMIC_ADD_INT32(PTR,VAL)   InterlockedAdd(PTR, VAL)
#else
	#error VTM_ATOMIC_ADD_INT32 not supported
#endif

/* atomic or */
#if defined(__GNUC__) || defined(__clang__)
	#define VTM_ATOMIC_OR_INT32(PTR, VAL)   __sync_or_and_fetch(PTR, VAL)
#elif defined(_MSC_VER)
	#define VTM_ATOMIC_OR_INT32(PTR, VAL)   InterlockedOr(PTR, VAL)
#else
	#error VTM_ATOMIC_OR_INT32 not supported
#endif

/* atomic and */
#if defined(__GNUC__) || defined(__clang__)
	#define VTM_ATOMIC_AND_INT32(PTR, VAL)   __sync_and_and_fetch(PTR, VAL)
#elif defined(_MSC_VER)
	#define VTM_ATOMIC_AND_INT32(PTR, VAL)   InterlockedAnd(PTR, VAL)
#else
	#error VTM_ATOMIC_AND_INT32 not supported
#endif

/* compare and swap */
#if defined(__GNUC__) || defined(__clang__)
	#define VTM_ATOMIC_CAS_INT32(PTR, OLD, VAL)   __sync_val_compare_and_swap(PTR, OLD, VAL)
#elif defined(_MSC_VER)
	#define VTM_ATOMIC_CAS_INT32(PTR, OLD, VAL)   InterlockedCompareExchange(PTR, VAL, OLD)
#else
	#error VTM_ATOMIC_CAS_INT32(PTR, OLD, VAL) not supported
#endif

/* atomic load */
#define VTM_ATOMIC_LOAD_INT32(PTR)  VTM_ATOMIC_ADD_INT32(PTR, 0)

/* store zero */
#define VTM_ATOMIC_ZERO_INT32(PTR)  VTM_ATOMIC_AND_INT32(PTR, 0)

/* ########## ATOMIC FLAG ########## */
typedef VTM_ATOMIC_INT32_TYPE vtm_atomic_flag;

#define vtm_atomic_flag_init(VAR, VAL)     (VAR) = (VAL) ? 1 : 0;
#define vtm_atomic_flag_isset(VAR)         VTM_ATOMIC_LOAD_INT32(&(VAR))
#define vtm_atomic_flag_set(VAR)    (void) VTM_ATOMIC_OR_INT32(&(VAR), 1)
#define vtm_atomic_flag_unset(VAR)  (void) VTM_ATOMIC_AND_INT32(&(VAR), 0)

#endif /* VTM_UTIL_ATOMIC_H_ */
