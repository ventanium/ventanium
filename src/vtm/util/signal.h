/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file signal.h
 *
 * @brief Signal handling
 */

#ifndef VTM_UTIL_SIGNAL_H_
#define VTM_UTIL_SIGNAL_H_

#include <vtm/core/api.h>

#ifdef __cplusplus
extern "C" {
#endif

enum vtm_signal_type
{
	VTM_SIG_HUP,
	VTM_SIG_INT,
	VTM_SIG_TERM,
	VTM_SIG_SEGV,
	VTM_SIG_PIPE,
	VTM_SIG_USR1,
	VTM_SIG_USR2
};

typedef void(*vtm_signal_fn)(int psig);

/**
 * Installs a signal handler for given signal.
 *
 * @param sig signal for which the handler function should be set
 * @param func the function that should be called when the signal occurs
 * @return VTM_OK if the signal handler was installed
 * @return VTM_E_NOT_SUPPORTED if the signal is not supported on this
 *         platform
 */
VTM_API int vtm_signal_set_handler(enum vtm_signal_type sig, vtm_signal_fn func);

/**
 * Blocks the given signal.
 *
 * @param sig the signal which should be blocked
 * @return VTM_OK if the call succeded
 * @return VTM_E_NOT_SUPPORTED if the signal is not supported on this
 *         platform
 * @return VTM_ERROR if the signal could not be blocked
 */
VTM_API int vtm_signal_block(enum vtm_signal_type sig);

/**
 * Restores default handler for the given signal.
 *
 * @param sig the signal for which the default handler should be restored
 * @return VTM_OK if the call succeded
 * @return VTM_E_NOT_SUPPORTED if the signal is not supported on this
 *         platform
 * @return VTM_ERROR if the default handler could not be restored
 */
VTM_API int vtm_signal_default(enum vtm_signal_type sig);

/**
 * Converts platform specific signal number to enum.
 *
 * @param psig the signal number
 * @param[out] out the converted signal enum value
 * @return VTM_OK if the conversion succeeded
 * @return VTM_E_NOT_SUPPORTED if the signal is not supported on this
 *         platform
 */
VTM_API int vtm_signal_convert_from_os(int psig, enum vtm_signal_type *out);

/**
 * Converts enum to platform specific signal number.
 *
 * @param sig the signal that should be converted
 * @param[out] out the converted signal number
 * @return VTM_OK if the conversion succeeded
 * @return VTM_E_NOT_SUPPORTED if the signal is not supported on this
 *         platform
 */
VTM_API int vtm_signal_convert_to_os(enum vtm_signal_type sig, int *out);

/**
 * Safe way to print to stdout from signal handler.
 *
 * @param str the string to print
 */
VTM_API void vtm_signal_safe_puts(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* VTM_UTIL_SIGNAL_H_ */
