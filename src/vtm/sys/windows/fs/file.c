/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/fs/file.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vtm/core/error.h>

#if defined(__MINGW32__) && defined(__STRICT_ANSI__)
extern int _fileno(FILE*);
#endif

int vtm_file_get_fattr(FILE *fp, int *attr)
{
	int fd, hints;
	struct _stat buf;

	fd = _fileno(fp);

	if (_fstat(fd, &buf) != 0)
		return VTM_E_IO_UNKNOWN;

	hints = 0;

	if (buf.st_mode & _S_IFDIR)
		hints |= VTM_FILE_ATTR_DIR;

	if (buf.st_mode & _S_IFREG)
		hints |= VTM_FILE_ATTR_REG;

	*attr = hints;

	return VTM_OK;
}
