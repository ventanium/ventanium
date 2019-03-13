/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file json.h
 *
 * @brief JSON encoding
 */

#ifndef VTM_UTIL_JSON_H_
#define VTM_UTIL_JSON_H_

#include <vtm/core/api.h>
#include <vtm/core/buffer.h>
#include <vtm/core/dataset.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Encodes the content of a dataset as JSON.
 *
 * @param ds the input dataset
 * @param[out] buf holds the JSON string
 * @return VTM_OK if the encoding was successful
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_json_encode_ds(vtm_dataset *ds, struct vtm_buf *buf);

#ifdef __cplusplus
}
#endif

#endif /* VTM_UTIL_JSON_H_ */
