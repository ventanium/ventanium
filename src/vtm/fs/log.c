/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "log.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h> /* vsnprintf() */
#include <string.h>
#include <time.h>

#include <vtm/core/error.h>
#include <vtm/core/format.h>
#include <vtm/core/types.h>
#include <vtm/util/mutex.h>
#include <vtm/util/process.h>
#include <vtm/util/time.h>

#define VTM_LOG_DEFAULT_MAX_LINES 100000
#define VTM_LOG_MAX_LINE_LENGTH 2048

#define VTM_LOG_HAS_LEVEL(LOG, LEVEL) (LOG->level >= LEVEL)

static void vtm_log_write_internal(vtm_log *log, enum vtm_log_level level, const char *msg);
static void vtm_log_check_size(vtm_log *log);
static void vtm_log_rotate(vtm_log *log);

static const char* VTM_LOG_LABEL[] = {
	"ERROR",
	"WARN",
	"INFO",
	"VERBOSE",
	"DEBUG"
};

struct vtm_log
{
	enum vtm_log_level level;
	int hints;
	vtm_mutex *mtx;
	FILE *fp;
	char *file;
	char fmt_buf[VTM_LOG_MAX_LINE_LENGTH];
	unsigned long max_lines;
	unsigned long cur_lines;
	unsigned long pid;
	uint64_t ts_rotate;
	uint64_t cnt_rotate;

};

vtm_log* vtm_log_open(const char *path, const char *name, int hints)
{
	vtm_log *log;

	log = malloc(sizeof(vtm_log));
	if (!log) {
		vtm_err_oom();
		return NULL;
	}

	log->mtx = vtm_mutex_new();
	if (!log->mtx) {
		free(log);
		return NULL;
	}

	log->level = VTM_LOG_INFO;
	log->hints = hints;
	log->max_lines = VTM_LOG_DEFAULT_MAX_LINES;
	log->cur_lines = 0;
	log->pid = vtm_process_get_current_id();
	log->ts_rotate = 0;
	log->cnt_rotate = 0;
	log->file = NULL;
	log->fp = NULL;

	if (hints & VTM_LOG_HINT_FILE) {
		log->file = malloc(strlen(path) + strlen(name) + 6);
		if (!log->file) {
			vtm_err_oom();
			free(log);
			return NULL;
		}

		strcpy(log->file, path);
		strcat(log->file, "/");
		strcat(log->file, name);
		strcat(log->file, ".log");

		log->fp = fopen(log->file, "a");
		if (!log->fp) {
			vtm_err_setf(VTM_E_IO_UNKNOWN, "could not open logfile: %s", log->file);
			free(log->file);
			free(log);
			return NULL;
		}
	}

	return log;
}

void vtm_log_close(vtm_log *log)
{
	fclose(log->fp);
	vtm_mutex_free(log->mtx);
	free(log->file);
	free(log);
}

void vtm_log_set_level(vtm_log *log, enum vtm_log_level level)
{
	log->level = level;
}

void vtm_log_set_max_lines(vtm_log *log, unsigned long lines)
{
	log->max_lines = lines;
}

void vtm_log_write(vtm_log *log, enum vtm_log_level level, const char *msg)
{
	if (!VTM_LOG_HAS_LEVEL(log, level))
		return;

	vtm_mutex_lock(log->mtx);
	vtm_log_write_internal(log, level, msg);
	vtm_mutex_unlock(log->mtx);
}

void vtm_log_writef(vtm_log *log, enum vtm_log_level level, const char *fmt, ...)
{
	if (!VTM_LOG_HAS_LEVEL(log, level))
		return;

	vtm_mutex_lock(log->mtx);

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(log->fmt_buf, VTM_LOG_MAX_LINE_LENGTH, fmt, ap);
	va_end(ap);

	vtm_log_write_internal(log, level, log->fmt_buf);
	vtm_mutex_unlock(log->mtx);
}

static void vtm_log_write_internal(vtm_log *log, enum vtm_log_level level, const char *msg)
{
	char buf[32];
	size_t buf_used;
	FILE *console;
	struct tm *tinfo;
	uint64_t now_micros;
	time_t now_seconds;

	now_micros = vtm_time_current_micros();
	now_seconds = (time_t) (now_micros / 1000000);
	tinfo = localtime(&now_seconds);

	buf_used = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tinfo);
	buf[buf_used++] = '.';
	sprintf(&buf[buf_used], "%06u", (int) (now_micros % 1000000));

	/* console */
	if (log->hints & VTM_LOG_HINT_CONSOLE) {
		console = (level == VTM_LOG_ERROR) ? stderr : stdout;
		fprintf(console, "%s [%lu] %s: %s\n", buf, log->pid, VTM_LOG_LABEL[level], msg);
	}

	/* file */
	if (log->fp) {
		fprintf(log->fp, "%s [%lu] %s: %s\n", buf, log->pid, VTM_LOG_LABEL[level], msg);
		fflush(log->fp);
		vtm_log_check_size(log);
	}
}

static void vtm_log_check_size(vtm_log *log)
{
	if (++log->cur_lines < log->max_lines)
		return;

	vtm_log_rotate(log);
}

static void vtm_log_rotate(vtm_log *log)
{
	char *old;
	uint64_t millis;
	size_t pos;

	fclose(log->fp);
	millis = vtm_time_current_millis();

	pos = strlen(log->file);
	old = malloc(pos + VTM_FMT_CHARS_INT64 * 2 + 3); // 3 => '.' + '.' + '\0'
	if (!old) {
		vtm_err_oom();
		log->fp = NULL;
		return;
	}
	memcpy(old, log->file, pos);

	old[pos++] = '.';
	pos += vtm_fmt_uint64(&old[pos], millis);

	if (millis == log->ts_rotate) {
		log->cnt_rotate++;
		old[pos++] = '.';
		pos += vtm_fmt_uint64(&old[pos], log->cnt_rotate);
	}
	else {
		log->cnt_rotate = 0;
		log->ts_rotate = millis;
	}

	old[pos] = '\0';

	rename(log->file, old);
	free(old);

	log->cur_lines = 0;
	log->fp = fopen(log->file, "a");
	if (!log->fp)
		fprintf(stderr, "Error: could not reopen logfile: %s\n", log->file);
}
