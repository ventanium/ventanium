/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file http_file.h
 *
 * @brief File serving
 */

#ifndef VTM_NET_HTTP_HTTP_FILE_H_
#define VTM_NET_HTTP_HTTP_FILE_H_

#include <stdio.h> /* FILE */
#include <vtm/core/api.h>
#include <vtm/net/http/http_response.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Serves the contents of a file as response body.
 *
 * This call prepares the response so that when the response is sent,
 * the contents of the file stream are included as body part.
 *
 * @param res the response where the file should be served
 * @param name the of the file
 * @param fp the already opened file stream
 * @return VTM_OK if the file serving request was embedded into the response
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_http_file_serve(vtm_http_res *res, const char *name, FILE *fp);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_FILE_H_ */
