/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "system.h"

#include <vtm/core/types.h>

enum vtm_byteorder vtm_sys_get_byteorder(void)
{
	const uint32_t num = 0x0a0b0c0d;

	if (*((uint8_t*) &num) == 0x0d)
		return VTM_BYTEORDER_LE;

	return VTM_BYTEORDER_BE;
}
