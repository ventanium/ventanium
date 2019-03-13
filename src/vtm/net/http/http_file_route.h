/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file http_file_route.h
 *
 * @brief Route for serving static files
 */

#ifndef VTM_NET_HTTP_HTTP_FILE_ROUTE_H_
#define VTM_NET_HTTP_HTTP_FILE_ROUTE_H_

#include <vtm/core/api.h>
#include <vtm/net/http/http_route.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a route for serving static files from given root directory.
 *
 * For example if the root directory is set to "/srv/static/" and the route
 * is bound at "/files/", a HTTP request to path "/files/subdir/a.txt" will
 * serve "/srv/static/subdir/a.txt".
 *
 * @param fs_root the root directory
 * @return http route if call succeeded
 * @return NULL if an error occured
 */
VTM_API struct vtm_http_route* vtm_http_file_rt_new(const char *fs_root);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_FILE_ROUTE_H_ */
