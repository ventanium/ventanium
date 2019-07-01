/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "error.h"

#include <stdarg.h> /* va_start() */
#include <vtm/core/lang.h>

#define VTM_ERR_STR_BASE        "VTM-Error %d (%s) at %s:%lu"
#define VTM_ERR_STR_EXCL_MSG    VTM_ERR_STR_BASE "\n"
#define VTM_ERR_STR_INCL_MSG    VTM_ERR_STR_BASE ": %s\n"

struct vtm_err
{
	int             code;
	const char      *name;
	const char      *file;
	unsigned long   line;
	char            msg[VTM_ERR_MAX_MSG_LEN];
};

static VTM_THREAD_LOCAL struct vtm_err err;

int vtm_err_setf_intl(int code, const char *name, const char *file, unsigned long line, const char *msg, ...)
{
	va_list ap;

	err.code = code;
	err.name = name;
	err.file = file;
	err.line = line;

	if (msg) {
		va_start(ap, msg);
		vsnprintf(err.msg, sizeof(err.msg), msg, ap);
		va_end(ap);
	}
	else {
		err.msg[0] = '\0';
	}

	return code;
}

int vtm_err_get_code(void)
{
	return err.code;
}

const char* vtm_err_get_name(void)
{
	return err.name;
}

const char* vtm_err_get_msg(void)
{
	return err.msg;
}

const char* vtm_err_get_file(void)
{
	return err.file;
}

unsigned long vtm_err_get_line(void)
{
	return err.line;
}

void vtm_err_print(void)
{
	const char *template;

	template = err.msg[0] ? VTM_ERR_STR_INCL_MSG : VTM_ERR_STR_EXCL_MSG;

	fprintf(stderr, template, err.code, err.name, err.file, err.line, err.msg);
}
