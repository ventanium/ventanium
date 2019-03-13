/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file lang.h
 *
 * @brief Compiler specific definitions
 */

#ifndef VTM_CORE_LANG_H_
#define VTM_CORE_LANG_H_

/* thread local storage */
#if defined(__GNUC__) || defined(__clang__)
	#define VTM_THREAD_LOCAL   __thread
#elif defined(_MSC_VER)
	#define VTM_THREAD_LOCAL   __declspec(thread)
#else
	#error VTM_THREAD_LOCAL not supported
#endif

/* inlining */
#if defined(__GNUC__) || defined(__clang__)
	#define VTM_INLINE         inline
#elif defined(_MSC_VER)
	#define VTM_INLINE         __inline
#else
	#define VTM_INLINE
#endif

/* branch prediction */
#if defined(__GNUC__) || defined(__clang__)
	#define VTM_LIKELY(C)      __builtin_expect(!!(C), 1)
	#define VTM_UNLIKELY(C)    __builtin_expect(!!(C), 0)
#else
	#define VTM_LIKELY(C)      (C)
	#define VTM_UNLIKELY(C)    (C)
#endif

#endif /* VTM_CORE_LANG_H_ */
