/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file mime.h
 *
 * @brief Determining MIME type of files
 */

#ifndef VTM_FS_MIME_H_
#define VTM_FS_MIME_H_

#include <vtm/core/api.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VTM_MIME_TEXT_PLAIN         "text/plain"
#define VTM_MIME_TEXT_HTML          "text/html"

#define VTM_MIME_IMAGE_JPEG         "image/jpeg"
#define VTM_MIME_IMAGE_PNG          "image/png"

#define VTM_MIME_APP_JAVASCRIPT     "application/javascript"
#define VTM_MIME_APP_JSON           "application/json"

/**
 * Determines the (presumed) MIME type for the given filename.
 *
 * This function internally calls vtm_mime_type_for_ext().
 *
 * @param filename
 * @return MIME type string if a corresponding MIME type was found
 * @return NULL if no suitable MIME type was found
 */
VTM_API const char* vtm_mime_type_for_name(const char *filename);

/**
 * Determines the (presumed) MIME type for the given file extension.
 *
 * @param ext the extension
 * @return MIME type string if a corresponding MIME type was found
 * @return NULL if no suitable MIME type was found
 */
VTM_API const char* vtm_mime_type_for_ext(const char *ext);

#ifdef __cplusplus
}
#endif

#endif /* VTM_FS_MIME_H_ */
