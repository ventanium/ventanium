/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/util/process.h>

#include <windows.h>

unsigned long vtm_process_get_current_id()
{
	return (unsigned long) GetCurrentProcessId();
}
