/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/util/process.h>

#include <sys/types.h>
#include <unistd.h>

unsigned long vtm_process_get_current_id()
{
	return (unsigned long) getpid();
}
