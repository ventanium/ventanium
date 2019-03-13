/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include <vtm/fs/path.h>

#include <stdlib.h> /* malloc() */
#include <windows.h>
#include <vtm/core/error.h>

int vtm_path_get_real(const char *path, char **out_resolved)
{
	char *buffer;
	DWORD cnt;
	
	buffer = malloc(MAX_PATH);
	if (!buffer) {
		vtm_err_oom();
		return vtm_err_get_code();
	}
	
	cnt = GetFullPathNameA(path, MAX_PATH, buffer, NULL);
	if (cnt > MAX_PATH || cnt <= 0) {
		free(buffer);
		*out_resolved = NULL;
		return VTM_E_IO_UNKNOWN;
	}
	
	*out_resolved = buffer;
	
	return VTM_OK;
}
