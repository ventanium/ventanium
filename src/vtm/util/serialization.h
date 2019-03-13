/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file serialization.h
 *
 * @brief Serialization helper
 */

#ifndef VTM_UTIL_SERIALIZATION_H_
#define VTM_UTIL_SERIALIZATION_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Serializes a float value so that it can be stored
 * in 4 bytes.
 *
 * @param in the float that should be packed
 * @param[out] out holds the serialized data
 * @return VTM_OK if the packing was successful
 * @return VTM_E_INVALID_ARG if input is NaN
 */
VTM_API int vtm_pack_float(float in, uint32_t *out);

/**
 * Serializes a double value so that it can be stored
 * in 8 bytes.
 *
 * @param in the double that should be packed
 * @param[out] out holds the serialized data
 * @return VTM_OK if the packing was successful
 * @return VTM_E_INVALID_ARG if input is NaN
 */
VTM_API int vtm_pack_double(double in, uint64_t *out);

/**
 * Deserializes a 4 byte packed float value.
 *
 * @param in the packed data
 * @param[out] out the unpacked float value
 * @return VTM_OK if the unpacking was successful
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_unpack_float(uint32_t in, float *out);

/**
 * Deserializes a 8 byte packed double value.
 *
 * @param in the packed data
 * @param[out] out the unpacked double value
 * @return VTM_OK if the unpacking was successful
 * @return VTM_ERROR if an error occured
 */
VTM_API int vtm_unpack_double(uint64_t in, double *out);

#ifdef __cplusplus
}
#endif

#endif /* VTM_UTIL_SERIALIZATION_H_ */
