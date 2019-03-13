/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file error.h
 *
 * @brief Error handling and codes
 */

#ifndef VTM_CORE_ERROR_H_
#define VTM_CORE_ERROR_H_

#include <stdlib.h> /* abort() */
#include <stdio.h> /* fprintf() */
#include <vtm/core/api.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ########## CODES ########## */

#define VTM_OK                         0
#define VTM_ERROR                     -1

#define VTM_E_MALLOC                -100
#define VTM_E_MEMORY                -101
#define VTM_E_OUT_OF_RANGE          -102
#define VTM_E_NOT_SUPPORTED         -103
#define VTM_E_NOT_HANDLED           -104
#define VTM_E_NOT_FOUND             -105
#define VTM_E_ASSERT_FAILED         -106
#define VTM_E_INVALID_ARG           -107
#define VTM_E_INVALID_STATE         -108
#define VTM_E_OVERFLOW              -109
#define VTM_E_MAX_REACHED           -110
#define VTM_E_INTERRUPTED           -111
#define VTM_E_PERMISSION            -112

#define VTM_E_IO_UNKNOWN            -200
#define VTM_E_IO_EOF                -201
#define VTM_E_IO_AGAIN              -202
#define VTM_E_IO_CANCELED           -203
#define VTM_E_IO_CLOSED             -204
#define VTM_E_IO_TIMEOUT            -205
#define VTM_E_IO_FILE_NOT_FOUND     -206
#define VTM_E_IO_PARTIAL            -207
#define VTM_E_IO_PROTOCOL           -208

/* ########## ERROR ########## */

#define VTM_ERR_MAX_MSG_LEN          512

#define vtm_err_set(CODE)           vtm_err_setf_intl(CODE, #CODE, __FILE__, __LINE__, NULL)
#define vtm_err_sets(CODE,STR)      vtm_err_setf_intl(CODE, #CODE, __FILE__, __LINE__, STR)
#define vtm_err_setf(CODE,STR,...)  vtm_err_setf_intl(CODE, #CODE, __FILE__, __LINE__, STR, __VA_ARGS__)

VTM_API int vtm_err_setf_intl(int code, const char *name, const char *file, unsigned long line, const char *msg, ...);

VTM_API int vtm_err_get_code(void);
VTM_API const char* vtm_err_get_name(void);
VTM_API const char* vtm_err_get_msg(void);
VTM_API const char* vtm_err_get_file(void);
VTM_API unsigned long vtm_err_get_line(void);

VTM_API void vtm_err_print(void);

/* ########## ABORT ########## */

#define VTM_ABORT(STR)                                     \
	do {                                                   \
		fprintf(stderr, "VTM-Abort at " __FILE__ ":%d: "   \
			STR "\n", __LINE__);                           \
		abort();                                           \
	} while (0)

#define VTM_ABORT_FATAL()        VTM_ABORT("Fatal error")
#define VTM_ABORT_NO_MEMORY      VTM_ABORT("Could not allocate memory")
#define VTM_ABORT_NOT_SUPPORTED  VTM_ABORT("Not supported")
#define VTM_ABORT_NOT_REACHABLE  VTM_ABORT("Should not be reached")

/* ########## ASSERT ########## */

#define VTM_ASSERT(COND)                                   \
	do {                                                   \
		if (!(COND))                                       \
			VTM_ABORT("Assertion failed: " #COND);         \
	} while (0)

/* ########## OUT OF MEMORY ########## */

#ifndef VTM_OPT_NO_OOM_ABORT
#define vtm_err_oom()       VTM_ABORT_NO_MEMORY
#else
#define vtm_err_oom()       vtm_err_set(VTM_E_MALLOC)
#endif

#ifdef __cplusplus
}
#endif

#endif /* VTM_CORE_ERROR_H_ */
