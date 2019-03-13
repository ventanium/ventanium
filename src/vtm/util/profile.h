/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_UTIL_PROFILE_H_
#define VTM_UTIL_PROFILE_H_

#include <stdio.h> /* fprintf() */
#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/util/time.h>

#define VTM_PROFILE_STRUCT                             \
	struct {                                           \
		uint64_t count;                                \
		uint64_t sum;                                  \
		uint64_t begin;                                \
		uint64_t end;                                  \
	}

#define VTM_PROFILE_BEGIN(NAME)                        \
	NAME.begin = vtm_time_current_micros()

#define VTM_PROFILE_END(NAME, ITERATIONS)              \
	do {                                               \
		NAME.end = vtm_time_current_micros();          \
		NAME.sum += NAME.end - NAME.begin;             \
		NAME.count++;                                  \
		if (NAME.count == ITERATIONS) {                \
			fprintf(stderr, "Profiling took %" PRIu64  \
				" us for %" PRIu64 " iterations"       \
				" (%s:%d)\n", NAME.sum, ITERATIONS,    \
				__FILE__, __LINE__);                   \
			NAME.count = 0;                            \
			NAME.sum = 0;                              \
		}                                              \
	} while (0)

#endif /* VTM_UTIL_PROFILE_H_ */
