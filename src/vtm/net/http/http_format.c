/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "http_format.h"

#include <string.h> /* sprintf() */
#include <vtm/core/error.h>

static const char* VTM_HTTP_MONTH[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char* VTM_HTTP_DAY[] = {
	"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
};

int vtm_http_fmt_date(char *dst, size_t max_len, struct vtm_date *date)
{
	int rc;
	
	rc = snprintf(dst, max_len,
		"%.3s, %d %.3s %04d %02d:%02d:%02d GMT",
		VTM_HTTP_DAY[date->day_of_week],
		date->day_of_month,
		VTM_HTTP_MONTH[date->month],
		date->year,
		date->hour,
		date->minute,
		date->second);
	
	if (rc <= 0 || (size_t) rc >= max_len)
		return VTM_ERROR;
	
	return VTM_OK;
}
