/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/util/time.h>

#include <windows.h>

#define VTM_EPOCH_1601_USEC_DIFF	11644473600000000ull;

uint64_t vtm_time_current_millis()
{
	return vtm_time_current_micros() / 1000;
}

uint64_t vtm_time_current_micros()
{
	uint64_t ret;
	FILETIME tval;

	GetSystemTimeAsFileTime(&tval);

	ret  = (uint64_t) tval.dwHighDateTime << 32;
	ret += (uint64_t) tval.dwLowDateTime;
	ret /= 10;
	ret -= VTM_EPOCH_1601_USEC_DIFF;
	
	return ret;
}
