/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <stdio.h>
#include <vtm/fs/log.h>

static const char MSG1[] = "First log message";
static const char MSG2[] = "Second log message";
static const char MSG3[] = "Third log message";

static void test_log(void)
{
	vtm_log *log = vtm_log_open(".", "logtest", VTM_LOG_HINT_FILE);
	VTM_TEST_ASSERT(log != NULL, "logfile opened");
	
	vtm_log_write(log, VTM_LOG_INFO, MSG1);
	vtm_log_write(log, VTM_LOG_DEBUG, MSG2);
	vtm_log_writef(log, VTM_LOG_INFO, "%s %d", MSG3, 42);
	
	vtm_log_close(log);
	VTM_TEST_PASSED("log free");

	remove("./logtest.log");
}

extern void test_vtm_fs_log(void)
{
	VTM_TEST_LABEL("log");
	test_log();
}
