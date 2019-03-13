/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file api.h
 *
 * @brief Platform auto detection
 */

#ifndef VTM_CORE_API_H_
#define VTM_CORE_API_H_

/* OS auto detection */
#if !(defined(VTM_SYS_LINUX) || defined(VTM_SYS_BSD) || defined(VTM_SYS_DARWIN) || defined(VTM_SYS_WINDOWS))

	/* Linux */
	#if defined(__linux__) || defined(__gnu_linux__) || defined(__linux)
		#define VTM_SYS_LINUX    1
		#define VTM_SYS_UNIX     1
		#define VTM_HAVE_POSIX   1
	/* BSD */
	#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
		#define VTM_SYS_BSD      1
		#define VTM_SYS_UNIX     1
		#define VTM_HAVE_POSIX   1
	/* Darwin */
	#elif defined(__APPLE__) || defined(__MACH__)
		#define VTM_SYS_DARWIN   1
		#define VTM_SYS_UNIX     1
		#define VTM_HAVE_POSIX   1
	#elif defined(_WIN32)
		#define VTM_SYS_WINDOWS  1
	#endif

#endif

/* symbol export */
#if defined(VTM_SYS_WINDOWS) && defined(VTM_DLL_EXPORT)
	#define VTM_API              __declspec(dllexport)
#elif defined(VTM_SYS_WINDOWS) && defined(VTM_DLL_IMPORT)
	#define VTM_API              __declspec(dllimport)
#else
	#define VTM_API
#endif

#endif /* VTM_CORE_API_H_ */
