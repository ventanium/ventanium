/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include <vtm/fs/path.h>

#include <stdlib.h> /* realpath() */
#include <vtm/core/error.h>

int vtm_path_get_real(const char *path, char **out_resolved)
{
	*out_resolved = realpath(path, NULL);
	if (!*out_resolved)
		return vtm_err_set(VTM_E_IO_UNKNOWN);
		
	return VTM_OK;
}
