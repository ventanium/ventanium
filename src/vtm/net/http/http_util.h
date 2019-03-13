/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file http_util.h
 *
 * @brief Helper functions for authentication
 */

#ifndef VTM_NET_HTTP_HTTP_UTIL_H_
#define VTM_NET_HTTP_HTTP_UTIL_H_

#include <vtm/core/api.h>
#include <vtm/net/http/http_response.h>
#include <vtm/net/http/http_upgrade.h>
#include <vtm/net/http/http_file.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prepares the response so that basic authentication require headers are set.
 *
 * @param res the response that where the headers should be set
 * @param realm realm name for authentication
 * @return VTM_OK if the headers were successfully set
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_http_basic_auth_require(vtm_http_res *res, const char *realm);

/**
 * Extracts basic authentication information from the request.
 *
 * User and password string must be freed by the caller.
 *
 * @param req the request where the information should be extracted
 * @param[out] user will be filled with pointer to user name
 * @param[out] pass will be filled with pointer to password
 * @return VTM_OK if the information were successfully extracted
 * @return VTM_ERROR if the information could not be retrieved
 */
VTM_API int vtm_http_basic_auth_read(struct vtm_http_req *req, char **user, char **pass);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_UTIL_H_ */
