/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "time.h"

#include <time.h>
#include <vtm/core/error.h>

int vtm_date_now_utc(struct vtm_date *date)
{
	time_t time_now;
	struct tm *now;

	time_now = time(NULL);

#ifdef VTM_SYS_WINDOWS
	now = gmtime(&time_now);
#elif VTM_HAVE_POSIX
	struct tm result;
	now = gmtime_r(&time_now, &result);
#else
	#error need thread safe solution here
#endif	
	
	date->year = 1900 + now->tm_year;
	date->month = now->tm_mon;
	date->day_of_week = now->tm_wday == 0 ? 7 : now->tm_wday - 1;
	date->day_of_month = now->tm_mday;
	date->day_of_year = now->tm_yday;
	
	date->hour = now->tm_hour;
	date->minute = now->tm_min;
	date->second = now->tm_sec;
	
	date->ts = time_now;
	
	return VTM_OK;
}
