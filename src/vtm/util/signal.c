/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "signal.h"

#include <stdio.h> /* fwrite(() */
#include <string.h> /* memset() */
#include <signal.h>
#include <vtm/core/error.h>

int vtm_signal_set_handler(enum vtm_signal_type sig, vtm_signal_fn func)
{
	int rc;
	int psig;

	rc = vtm_signal_convert_to_os(sig, &psig);
	if (rc != VTM_OK)
		return rc;

#ifdef VTM_HAVE_POSIX
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = func;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	if (sigaction(psig, &sa, NULL) != 0)
		return vtm_err_set(VTM_ERROR);
#else /* use std signal function */
	if (signal(psig, func) == SIG_ERR)
		return vtm_err_set(VTM_ERROR);
#endif

	return VTM_OK;
}

int vtm_signal_block(enum vtm_signal_type sig)
{
	return vtm_signal_set_handler(sig, SIG_IGN);
}

int vtm_signal_default(enum vtm_signal_type sig)
{
	return vtm_signal_set_handler(sig, SIG_DFL);
}

int vtm_signal_convert_from_os(int psig, enum vtm_signal_type *out)
{
	switch (psig) {
		case SIGINT:	*out = VTM_SIG_INT;  return VTM_OK;
		case SIGTERM:	*out = VTM_SIG_TERM; return VTM_OK;
		case SIGSEGV:	*out = VTM_SIG_SEGV; return VTM_OK;

#ifdef VTM_HAVE_POSIX
		case SIGHUP:	*out = VTM_SIG_HUP;	 return VTM_OK;
		case SIGPIPE:	*out = VTM_SIG_PIPE; return VTM_OK;
		case SIGUSR1:	*out = VTM_SIG_USR1; return VTM_OK;
		case SIGUSR2:	*out = VTM_SIG_USR2; return VTM_OK;
#endif

		default:		break;
	}

	return VTM_E_NOT_SUPPORTED;
}

int vtm_signal_convert_to_os(enum vtm_signal_type sig, int *out)
{
	switch (sig) {
		case VTM_SIG_INT:	*out = SIGINT;  return VTM_OK;
		case VTM_SIG_TERM:	*out = SIGTERM; return VTM_OK;
		case VTM_SIG_SEGV:	*out = SIGSEGV; return VTM_OK;

#ifdef VTM_HAVE_POSIX
		case VTM_SIG_HUP:	*out = SIGHUP;  return VTM_OK;
		case VTM_SIG_PIPE:	*out = SIGPIPE; return VTM_OK;
		case VTM_SIG_USR1:	*out = SIGUSR1; return VTM_OK;
		case VTM_SIG_USR2:	*out = SIGUSR2; return VTM_OK;
#endif

		default:			break;
	}

	return VTM_E_NOT_SUPPORTED;
}

void vtm_signal_safe_puts(const char *str)
{
	fwrite(str, strlen(str), 1, stdout);
}
