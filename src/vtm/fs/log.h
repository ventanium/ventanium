/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file log.h
 *
 * @brief Simple logging to a file and to console
 */

#ifndef VTM_FS_LOG_H_
#define VTM_FS_LOG_H_

#include <vtm/core/api.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VTM_LOG_HINT_CONSOLE       1  /**< log to stdout/stderr */
#define VTM_LOG_HINT_FILE          2  /**< log to file */

enum vtm_log_level
{
	VTM_LOG_ERROR = 0,
	VTM_LOG_WARN = 1,
	VTM_LOG_INFO = 2,
	VTM_LOG_VERBOSE = 3,
	VTM_LOG_DEBUG = 4
};

typedef struct vtm_log vtm_log;

/**
 * Opens a new log.
 *
 * The log is internally synchronized by a mutex and can be used
 * from multiple threads simultaneously.
 *
 * @param path where the log file should be stored. Can be NULL if
 *        VTM_LOG_HINT_FILE is not used
 * @param name of the logfile without extension. Can be NULL if
 *        VTM_LOG_HINT_FILE is not used
 * @param hints an OR'ed combination of the possible hints defined above.
 * @return log handle for use in the other functions
 * @return NULL if an error occured
 */
VTM_API vtm_log* vtm_log_open(const char *path, const char *name, int hints);

/**
 * Closes the log and frees all allocated resources.
 * @param log which shoud be closed
 */
VTM_API void vtm_log_close(vtm_log *log);

/**
 * Sets which messages should be logged.
 * @param log the log which should be updated
 * @param level the threshold up to which level messages are logged
 */
VTM_API void vtm_log_set_level(vtm_log *log, enum vtm_log_level level);

/**
 * Sets the maximum lines a log should write to one file.
 *
 * When the maximus is reached, the current logfile is renamed and a new
 * one is opened.
 *
 * @param log the log which should be updated
 * @param lines the new maximum
 */
VTM_API void vtm_log_set_max_lines(vtm_log *log, unsigned long lines);

/**
 * Logs a simple message.
 *
 * If the log level of the log is lower than the one given, the message
 * is ignored.
 *
 * @param log the log which should be used
 * @param level the level of the message
 * @param msg the message which should be logged
 */
VTM_API void vtm_log_write(vtm_log *log, enum vtm_log_level level, const char *msg);

/**
 * Logs a message with a format string and varargs.
 *
 * The syntax for the format string is the same as for printf().
 *
 * @param log the log which should be used
 * @param level the level of the message
 * @param fmt the message as format string
 */
VTM_API void vtm_log_writef(vtm_log *log, enum vtm_log_level level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* VTM_FS_LOG_H_ */
