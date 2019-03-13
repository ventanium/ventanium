/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file elem.h
 *
 * @brief Base type
 */

#ifndef VTM_CORE_ELEM_H_
#define VTM_CORE_ELEM_H_

#include <stdarg.h>
#include <vtm/core/api.h>
#include <vtm/core/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char* const VTM_ELEM_BOOL_TRUE;  /**< TRUE as string representation */
extern const char* const VTM_ELEM_BOOL_FALSE; /**< FALSE as string representation */

enum vtm_elem_type
{
	VTM_ELEM_NULL,     /**< special NULL value */
	VTM_ELEM_INT8,     /**< signed 8 bit integer */
	VTM_ELEM_UINT8,    /**< unsigned 8 bit integer */
	VTM_ELEM_INT16,    /**< signed 16 bit integer */
	VTM_ELEM_UINT16,   /**< unsigned 16 bit integer */
	VTM_ELEM_INT32,    /**< signed 32 bit integer */
	VTM_ELEM_UINT32,   /**< unsigned 32 bit integer */
	VTM_ELEM_INT64,    /**< signed 64 bit integer */
	VTM_ELEM_UINT64,   /**< unsigned 64 bit integer */
	VTM_ELEM_BOOL,     /**< boolean value */
	VTM_ELEM_CHAR,     /**< char (signed or unsigned depends on platform) */
	VTM_ELEM_SCHAR,     /**< signed char */
	VTM_ELEM_UCHAR,    /**< unsigned char */
	VTM_ELEM_SHORT,    /**< signed short (width depends on platform) */
	VTM_ELEM_USHORT,   /**< unsigned short (width depends on platform) */
	VTM_ELEM_INT,      /**< signed integer (width depends on platform) */
	VTM_ELEM_UINT,     /**< unsigned integer (width depends on platform) */
	VTM_ELEM_LONG,     /**< signed long integer (width depends on platform) */
	VTM_ELEM_ULONG,    /**< unsigned long integer (width depends on platform) */
	VTM_ELEM_FLOAT,    /**< float value */
	VTM_ELEM_DOUBLE,   /**< double value */
	VTM_ELEM_STRING,   /**< NUL-terminated string */
	VTM_ELEM_POINTER   /**< raw pointer */
};

union vtm_elem
{
	int8_t          elem_int8;
	uint8_t         elem_uint8;
	int16_t         elem_int16;
	uint16_t        elem_uint16;
	int32_t         elem_int32;
	uint32_t        elem_uint32;
	int64_t         elem_int64;
	uint64_t        elem_uint64;
	bool            elem_bool;
	char            elem_char;
	signed char     elem_schar;
	unsigned char   elem_uchar;
	short           elem_short;
	unsigned short  elem_ushort;
	int             elem_int;
	unsigned int    elem_uint;
	long            elem_long;
	unsigned long   elem_ulong;
	float           elem_float;
	double          elem_double;
	void           *elem_pointer;
};

/**
 * Gets the size of an element type.
 *
 * @param type the type of element whose size should be returned
 * @return size of type in bytes
 */
VTM_API size_t vtm_elem_size(enum vtm_elem_type type);

/**
 * Parses an element from variadic argument list.
 *
 * @param type the type of the element that should be parsed
 * @param[out] dst parsed value
 * @param ap variadic argument list
 */
VTM_API void vtm_elem_parse(enum vtm_elem_type type, union vtm_elem *dst, va_list *ap);

VTM_API int8_t vtm_elem_as_int8(enum vtm_elem_type type, const union vtm_elem *src);
VTM_API uint8_t vtm_elem_as_uint8(enum vtm_elem_type type, const union vtm_elem *src);

VTM_API int16_t vtm_elem_as_int16(enum vtm_elem_type type, const union vtm_elem *src);
VTM_API uint16_t vtm_elem_as_uint16(enum vtm_elem_type type, const union vtm_elem *src);

VTM_API int32_t vtm_elem_as_int32(enum vtm_elem_type type, const union vtm_elem *src);
VTM_API uint32_t vtm_elem_as_uint32(enum vtm_elem_type type, const union vtm_elem *src);

VTM_API int64_t vtm_elem_as_int64(enum vtm_elem_type type, const union vtm_elem *src);
VTM_API uint64_t vtm_elem_as_uint64(enum vtm_elem_type type, const union vtm_elem *src);

VTM_API bool vtm_elem_as_bool(enum vtm_elem_type type, const union vtm_elem *src);

VTM_API char vtm_elem_as_char(enum vtm_elem_type type, const union vtm_elem *src);
VTM_API signed char vtm_elem_as_schar(enum vtm_elem_type type, const union vtm_elem *src);
VTM_API unsigned char vtm_elem_as_uchar(enum vtm_elem_type type, const union vtm_elem *src);

VTM_API short vtm_elem_as_short(enum vtm_elem_type type, const union vtm_elem *src);
VTM_API unsigned short vtm_elem_as_ushort(enum vtm_elem_type type, const union vtm_elem *src);

VTM_API int vtm_elem_as_int(enum vtm_elem_type type, const union vtm_elem *src);
VTM_API unsigned int vtm_elem_as_uint(enum vtm_elem_type type, const union vtm_elem *src);

VTM_API long vtm_elem_as_long(enum vtm_elem_type type, const union vtm_elem *src);
VTM_API unsigned long vtm_elem_as_ulong(enum vtm_elem_type type, const union vtm_elem *src);

VTM_API float vtm_elem_as_float(enum vtm_elem_type type, const union vtm_elem *src);
VTM_API double vtm_elem_as_double(enum vtm_elem_type type, const union vtm_elem *src);

VTM_API char* vtm_elem_as_str(enum vtm_elem_type type, const union vtm_elem *src);
VTM_API void* vtm_elem_as_ptr(enum vtm_elem_type type, const union vtm_elem *src);

/**
 * Checks if the given elements are equal.
 *
 * @param e1 the first element
 * @param e2 the second element
 * @return true if e1 is equal to e2
 * @return false otherwise
 */
typedef bool (*vtm_elem_cmp_fn)(union vtm_elem *e1, union vtm_elem *e2);

/**
 * Gets a function pointer to the appropriate element compare function.
 *
 * @param type the element type
 * @return function pointer to element compare function
 */
VTM_API vtm_elem_cmp_fn vtm_elem_get_cmp_fn(enum vtm_elem_type type);

/* Compare functions */
VTM_API bool vtm_elem_cmp_int8(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_uint8(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_int16(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_uint16(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_int32(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_uint32(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_int64(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_uint64(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_bool(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_char(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_schar(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_uchar(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_short(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_ushort(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_int(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_uint(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_long(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_ulong(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_float(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_double(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_str(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_strcase(union vtm_elem *e1, union vtm_elem *e2);
VTM_API bool vtm_elem_cmp_ptr(union vtm_elem *e1, union vtm_elem *e2);

#ifdef __cplusplus
}
#endif

#endif /* VTM_CORE_ELEM_H_ */
