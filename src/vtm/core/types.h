/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file types.h
 *
 * @brief Basic type definitions
 */

#ifndef VTM_CORE_TYPES_H_
#define VTM_CORE_TYPES_H_

#include <stddef.h>    /* size_t */
#include <stdbool.h>   /* bool */
#include <stdint.h>    /* fixed sized integers */
#include <inttypes.h>  /* printf symbols for fixed size integers */
#include <limits.h>    /* type max values */
#include <vtm/core/api.h>

 /* ssize_t */
#ifdef VTM_SYS_WINDOWS
	#include <basetsd.h>
	#ifdef _WIN64
		#define SSIZE_MAX _I64_MAX
	#else
		#define SSIZE_MAX LONG_MAX
	#endif

	#ifdef _MSC_VER
		typedef SSIZE_T ssize_t;
	#elif __MINGW32__
		#include <sys/types.h>
	#else
		#error ssize_t undefined
	#endif
#else
	#include <sys/types.h>
#endif

#endif /* VTM_CORE_TYPES_H_ */
