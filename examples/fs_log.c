/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/core/error.h>
#include <vtm/fs/log.h>

int main(int argc, char **argv)
{
	vtm_log *log;

	/*
	 * Opens the logfile in current directory with the name
	 * "example.log".
	 * The log level is set to INFO by default, that means only log messages
	 * with log level ERROR(0), WARN(1), INFO(2) are considered.
	 * Log levels VERBOSE(3) and DEBUG(4) are ignored with this setting.
	 */
	log = vtm_log_open(".", "example", VTM_LOG_HINT_CONSOLE | VTM_LOG_HINT_FILE);
	if (!log) {
		/* log file could not be opened or other error */
		vtm_err_print();
		return EXIT_FAILURE;
	}

	vtm_log_write (log, VTM_LOG_ERROR, "Some unknown error happened");
	vtm_log_writef(log, VTM_LOG_WARN, "Warning in line %d", __LINE__);
	vtm_log_write (log, VTM_LOG_INFO, "Log is working");

	/* the following messages should be ignored by the log */
	vtm_log_write (log, VTM_LOG_VERBOSE, "Giving some more details");
	vtm_log_writef(log, VTM_LOG_DEBUG, "Value of log pointer is %p", log);

	/* close the log file */
	vtm_log_close(log);

	return 0;
}
