/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file variant.h
 *
 * @brief Variant structure
 */

#ifndef VTM_CORE_VARIANT_H_
#define VTM_CORE_VARIANT_H_

#include <vtm/core/api.h>
#include <vtm/core/elem.h>
#include <vtm/core/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Structure that can hold different data types */
struct vtm_variant
{
	/* public */
	union vtm_elem data;      /**< value storage */
	enum vtm_elem_type type;  /**< type of stored value */

	/* internal */
	const char *str;
};

/**
 * Initializes a new empty variant.
 *
 * @param var the variant that should be initalized
 */
VTM_API void vtm_variant_init(struct vtm_variant *var);

/**
 * Releases the variant and all allocated resources.
 *
 * @param var the variant that should be released
 */
VTM_API void vtm_variant_release(struct vtm_variant *var);

/* value conversion */
VTM_API int8_t vtm_variant_as_int8(const struct vtm_variant *var);
VTM_API uint8_t vtm_variant_as_uint8(const struct vtm_variant *var);

VTM_API int16_t vtm_variant_as_int16(const struct vtm_variant *var);
VTM_API uint16_t vtm_variant_as_uint16(const struct vtm_variant *var);

VTM_API int32_t vtm_variant_as_int32(const struct vtm_variant *var);
VTM_API uint32_t vtm_variant_as_uint32(const struct vtm_variant *var);

VTM_API int64_t vtm_variant_as_int64(const struct vtm_variant *var);
VTM_API uint64_t vtm_variant_as_uint64(const struct vtm_variant *var);

VTM_API bool vtm_variant_as_bool(const struct vtm_variant *var);

VTM_API char vtm_variant_as_char(const struct vtm_variant *var);
VTM_API signed char vtm_variant_as_schar(const struct vtm_variant *var);
VTM_API unsigned char vtm_variant_as_uchar(const struct vtm_variant *var);

VTM_API short vtm_variant_as_short(const struct vtm_variant *var);
VTM_API unsigned short vtm_variant_as_ushort(const struct vtm_variant *var);

VTM_API int vtm_variant_as_int(const struct vtm_variant *var);
VTM_API unsigned int vtm_variant_as_uint(const struct vtm_variant *var);

VTM_API long vtm_variant_as_long(const struct vtm_variant *var);
VTM_API unsigned long vtm_variant_as_ulong(const struct vtm_variant *var);

VTM_API float vtm_variant_as_float(const struct vtm_variant *var);
VTM_API double vtm_variant_as_double(const struct vtm_variant *var);

VTM_API const char* vtm_variant_as_str(struct vtm_variant *var);
VTM_API const void* vtm_variant_as_blob(const struct vtm_variant *var);
VTM_API void* vtm_variant_as_ptr(const struct vtm_variant *var);

/* variant initializers */
VTM_API struct vtm_variant vtm_variant_from_null(void);
VTM_API struct vtm_variant vtm_variant_from_int8(int8_t val);
VTM_API struct vtm_variant vtm_variant_from_uint8(uint8_t val);
VTM_API struct vtm_variant vtm_variant_from_int16(int16_t val);
VTM_API struct vtm_variant vtm_variant_from_uint16(uint16_t val);
VTM_API struct vtm_variant vtm_variant_from_int32(int32_t val);
VTM_API struct vtm_variant vtm_variant_from_uint32(uint32_t val);
VTM_API struct vtm_variant vtm_variant_from_int64(int64_t val);
VTM_API struct vtm_variant vtm_variant_from_uint64(uint64_t val);
VTM_API struct vtm_variant vtm_variant_from_bool(bool val);
VTM_API struct vtm_variant vtm_variant_from_char(char val);
VTM_API struct vtm_variant vtm_variant_from_schar(signed char val);
VTM_API struct vtm_variant vtm_variant_from_uchar(unsigned char val);
VTM_API struct vtm_variant vtm_variant_from_short(short val);
VTM_API struct vtm_variant vtm_variant_from_ushort(unsigned short val);
VTM_API struct vtm_variant vtm_variant_from_int(int val);
VTM_API struct vtm_variant vtm_variant_from_uint(unsigned int val);
VTM_API struct vtm_variant vtm_variant_from_long(long val);
VTM_API struct vtm_variant vtm_variant_from_ulong(unsigned long val);
VTM_API struct vtm_variant vtm_variant_from_float(float val);
VTM_API struct vtm_variant vtm_variant_from_double(double val);
VTM_API struct vtm_variant vtm_variant_from_str(char *val);
VTM_API struct vtm_variant vtm_variant_from_blob(void *val);
VTM_API struct vtm_variant vtm_variant_from_ptr(void *val);

/* shorthand macros */
#define VTM_V_NULL()       vtm_variant_from_null()
#define VTM_V_INT8(VAL)    vtm_variant_from_int8(VAL)
#define VTM_V_UINT8(VAL)   vtm_variant_from_uint8(VAL)
#define VTM_V_INT16(VAL)   vtm_variant_from_int16(VAL)
#define VTM_V_UINT16(VAL)  vtm_variant_from_uint16(VAL)
#define VTM_V_INT32(VAL)   vtm_variant_from_int32(VAL)
#define VTM_V_UINT32(VAL)  vtm_variant_from_uint32(VAL)
#define VTM_V_INT64(VAL)   vtm_variant_from_int64(VAL)
#define VTM_V_UINT64(VAL)  vtm_variant_from_uint64(VAL)
#define VTM_V_BOOL(VAL)    vtm_variant_from_bool(VAL)
#define VTM_V_CHAR(VAL)    vtm_variant_from_char(VAL)
#define VTM_V_SCHAR(VAL)   vtm_variant_from_schar(VAL)
#define VTM_V_UCHAR(VAL)   vtm_variant_from_uchar(VAL)
#define VTM_V_SHORT(VAL)   vtm_variant_from_short(VAL)
#define VTM_V_USHORT(VAL)  vtm_variant_from_ushort(VAL)
#define VTM_V_INT(VAL)     vtm_variant_from_int(VAL)
#define VTM_V_UINT(VAL)    vtm_variant_from_uint(VAL)
#define VTM_V_LONG(VAL)    vtm_variant_from_long(VAL)
#define VTM_V_ULONG(VAL)   vtm_variant_from_ulong(VAL)
#define VTM_V_FLOAT(VAL)   vtm_variant_from_float(VAL)
#define VTM_V_DOUBLE(VAL)  vtm_variant_from_double(VAL)
#define VTM_V_STR(VAL)     vtm_variant_from_str(VAL)
#define VTM_V_BLOB(VAL)    vtm_variant_from_blob(VAL)
#define VTM_V_PTR(VAL)     vtm_variant_from_ptr(VAL)

#ifdef __cplusplus
}
#endif

#endif /* VTM_CORE_VARIANT_H_ */
