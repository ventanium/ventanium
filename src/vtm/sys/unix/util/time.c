/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/util/time.h>

#include <sys/time.h>

uint64_t vtm_time_current_millis()
{
	struct timeval tval;
	gettimeofday(&tval, NULL);
	return tval.tv_sec * 1000 + (tval.tv_usec / 1000);
}

uint64_t vtm_time_current_micros()
{
	struct timeval tval;
	gettimeofday(&tval, NULL);
	return tval.tv_sec * 1000000 + tval.tv_usec;
}
