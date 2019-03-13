/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file time.h
 *
 * @brief Time functions
 */

#ifndef VTM_UTIL_TIME_H_
#define VTM_UTIL_TIME_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vtm_date
{
	int             year;          /**< Year */
	unsigned int    month;         /**< Month (0-11) */
	unsigned int    day_of_week;   /**< Day of week (0-6, Monday=0) */
	unsigned int    day_of_month;  /**< Day of month (1-31) */
	unsigned int    day_of_year;   /**< Day of year, (0-365, 1. Jan=0) */

	unsigned char   hour;          /**< Hours (0-23) */
	unsigned char   minute;        /**< Minutes (0-59) */
	unsigned char   second;        /**< Seconds (0-60, leap second possible) */

	uint64_t        ts;            /**< Seconds since 1970-01-01 UTC */
};

/**
 * Gets current date in UTC.
 *
 * @param[out] date the date structure that is filled
 * @return VTM_OK if call succeeded
 */
VTM_API int vtm_date_now_utc(struct vtm_date *date);

/**
 * Get current timestamp in milliseconds.
 *
 * @return milliseconds since 1970-01-01 UTC
 */
VTM_API uint64_t vtm_time_current_millis();

/**
 * Get current timestamp in microseconds.
 *
 * @return microseconds since 1970-01-01 UTC
 */
VTM_API uint64_t vtm_time_current_micros();

#ifdef __cplusplus
}
#endif

#endif /* VTM_UTIL_TIME_H_ */
