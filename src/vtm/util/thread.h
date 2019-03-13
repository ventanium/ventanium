/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file thread.h
 *
 * @brief Threading support
 */

#ifndef VTM_UTIL_THREAD_H_
#define VTM_UTIL_THREAD_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/util/signal.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtm_thread vtm_thread;
typedef int (*vtm_thread_func)(void *arg);

/**
 * Starts a new thread.
 *
 * @param func the function which should be executed as new thread
 * @param arg the arguments which are passed to the new thread
 * @return a handle to the created thread
 * @return NULL if the thread could not be created
 */
VTM_API vtm_thread* vtm_thread_new(vtm_thread_func func, void *arg);

/**
 * Frees the thread and all allocated resources.
 *
 * @param th the thread which should be freed
 */
VTM_API void vtm_thread_free(vtm_thread *th);

/**
 * Sends a signal to a specific thread.
 *
 * This is not supported on all platforms.
 *
 * @param th the thread which should receive the signal
 * @param sig the signal which sould be sent
 * @return VTM_OK if the signal was successfully sent
 * @return VTM_E_NOT_SUPPORTED if operation not supported on current platform
 */
VTM_API int vtm_thread_signal(vtm_thread *th, enum vtm_signal_type sig);

/**
 * Trys to stop a running thread.
 *
 * This is not supported on all platforms.
 *
 * @param th the thread which should be stopped.
 * @return VTM_OK if the cancelation request was successful
 * @return VTM_E_NOT_SUPPORTED if operation not supported on current platform
 * @return VTM_ERROR if the cancelation was not successful
 */
VTM_API int vtm_thread_cancel(vtm_thread *th);

/**
 * Blocks until the given thread has finished.
 *
 * @param th the thread which should be joined
 * @return VTM_OK if the operation succeed
 * @return VTM_ERROR if the join was not successful
 */
VTM_API int vtm_thread_join(vtm_thread *th);

/**
 * Checks if given thread is running.
 *
 * @param th the thread which should be checked
 * @return true if the thread is running, false otherwise
 */
VTM_API bool vtm_thread_running(vtm_thread *th);

/**
 * Gets the result code of given thread.
 *
 * @param th the thread whose result is read
 * @return the result code of the thread
 */
VTM_API int vtm_thread_get_result(vtm_thread *th);

/**
 * Get the unique id of the given thread.
 *
 * Note: the format of the ids is not comparable between
 * different platforms.
 *
 * @param th the thread whose id should be read
 * @return the unique id
 */
VTM_API unsigned long vtm_thread_get_id(vtm_thread *th);

/**
 * Get the unique id of the current thread.
 *
 * Note: the format of the ids is not comparable between
 * different platforms.
 *
 * @return the unique id
 */
VTM_API unsigned long vtm_thread_get_current_id();

/**
 * Let the current thread sleep.
 *
 * It is not guaranteed that the thread sleeps the complete interval,
 * the sleep could be interrupted by a signal.
 *
 * @param millis the amount of milliseconds the thread should sleep
 */
VTM_API void vtm_thread_sleep(unsigned int millis);

#ifdef __cplusplus
}
#endif

#endif /* VTM_UTIL_THREAD_H_ */
