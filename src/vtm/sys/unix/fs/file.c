/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/fs/file.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vtm/core/error.h>

int vtm_file_get_fattr(FILE *fp, int *attr)
{
	int fd, hints;
	struct stat buf;

	fd = fileno(fp);

	if (fstat(fd, &buf) != 0)
		return VTM_E_IO_UNKNOWN;

	hints = 0;

	if (buf.st_mode & S_IFDIR)
		hints |= VTM_FILE_ATTR_DIR;

	if (buf.st_mode & S_IFREG)
		hints |= VTM_FILE_ATTR_REG;

	*attr = hints;

	return VTM_OK;
}
