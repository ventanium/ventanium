/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "elem.h"

#include <string.h>
#include <vtm/core/convert.h>
#include <vtm/core/error.h>
#include <vtm/core/string.h>

/* String representation for boolean false and true value */
const char* const VTM_ELEM_BOOL_TRUE = "TRUE";
const char* const VTM_ELEM_BOOL_FALSE = "FALSE";

size_t vtm_elem_size(enum vtm_elem_type type)
{
	switch (type) {
		case VTM_ELEM_NULL:     return sizeof(char);
		case VTM_ELEM_INT8:     return sizeof(int8_t);
		case VTM_ELEM_UINT8:    return sizeof(uint8_t);
		case VTM_ELEM_INT16:    return sizeof(int16_t);
		case VTM_ELEM_UINT16:   return sizeof(uint16_t);
		case VTM_ELEM_INT32:    return sizeof(int32_t);
		case VTM_ELEM_UINT32:   return sizeof(uint32_t);
		case VTM_ELEM_INT64:    return sizeof(int64_t);
		case VTM_ELEM_UINT64:   return sizeof(uint64_t);
		case VTM_ELEM_BOOL:     return sizeof(bool);
		case VTM_ELEM_CHAR:     return sizeof(char);
		case VTM_ELEM_SCHAR:    return sizeof(signed char);
		case VTM_ELEM_UCHAR:    return sizeof(unsigned char);
		case VTM_ELEM_SHORT:    return sizeof(short);
		case VTM_ELEM_USHORT:   return sizeof(unsigned short);
		case VTM_ELEM_INT:      return sizeof(int);
		case VTM_ELEM_UINT:     return sizeof(unsigned int);
		case VTM_ELEM_LONG:     return sizeof(long);
		case VTM_ELEM_ULONG:    return sizeof(unsigned long);
		case VTM_ELEM_FLOAT:    return sizeof(float);
		case VTM_ELEM_DOUBLE:   return sizeof(double);
		case VTM_ELEM_STRING:   return sizeof(char*);
		case VTM_ELEM_POINTER:  return sizeof(void*);
	}
	return 0;
}

void vtm_elem_parse(enum vtm_elem_type type, union vtm_elem *dst, va_list *ap)
{
	switch (type) {
		case VTM_ELEM_INT8:
			dst->elem_int8 = (int8_t) va_arg(*ap, int);
			break;

		case VTM_ELEM_UINT8:
			dst->elem_uint8 = (uint8_t) va_arg(*ap, unsigned int);
			break;

		case VTM_ELEM_INT16:
			dst->elem_int16 = (int16_t) va_arg(*ap, int);
			break;

		case VTM_ELEM_UINT16:
			dst->elem_uint16 = (uint16_t) va_arg(*ap, unsigned int);
			break;

		case VTM_ELEM_INT32:
			dst->elem_int32 = (int32_t) va_arg(*ap, int32_t);
			break;

		case VTM_ELEM_UINT32:
			dst->elem_uint32 = (uint32_t) va_arg(*ap, uint32_t);
			break;

		case VTM_ELEM_INT64:
			dst->elem_int64 = (int64_t) va_arg(*ap, int64_t);
			break;

		case VTM_ELEM_UINT64:
			dst->elem_uint64 = (uint64_t) va_arg(*ap, uint64_t);
			break;

		case VTM_ELEM_BOOL:
			dst->elem_bool = (bool) va_arg(*ap, int);
			break;

		case VTM_ELEM_CHAR:
			dst->elem_char = (char) va_arg(*ap, int);
			break;

		case VTM_ELEM_SCHAR:
			dst->elem_schar = (signed char) va_arg(*ap, int);
			break;

		case VTM_ELEM_UCHAR:
			dst->elem_uchar = (unsigned char) va_arg(*ap, unsigned int);
			break;

		case VTM_ELEM_SHORT:
			dst->elem_short = (short) va_arg(*ap, int);
			break;

		case VTM_ELEM_USHORT:
			dst->elem_ushort = (unsigned short) va_arg(*ap, unsigned int);
			break;

		case VTM_ELEM_INT:
			dst->elem_int = va_arg(*ap, int);
			break;

		case VTM_ELEM_UINT:
			dst->elem_uint = va_arg(*ap, unsigned int);
			break;

		case VTM_ELEM_LONG:
			dst->elem_long = va_arg(*ap, long);
			break;

		case VTM_ELEM_ULONG:
			dst->elem_ulong = va_arg(*ap, unsigned long);
			break;

		case VTM_ELEM_FLOAT:
			dst->elem_float = (float) va_arg(*ap, double);
			break;

		case VTM_ELEM_DOUBLE:
			dst->elem_double = va_arg(*ap, double);
			break;

		case VTM_ELEM_STRING:
			dst->elem_pointer = vtm_str_copy(va_arg(*ap, char*));
			break;

		case VTM_ELEM_POINTER:
			dst->elem_pointer = va_arg(*ap, void*);
			break;

		default:
			VTM_ABORT_NOT_SUPPORTED;
	}
}

/* elem conversion functions */

#define VTM_ELEM_CONV_NUMERIC(RTYPE)                                      \
	switch (type) {                                                       \
	case VTM_ELEM_INT8:     return (RTYPE) src->elem_int8;                \
	case VTM_ELEM_UINT8:    return (RTYPE) src->elem_uint8;               \
	case VTM_ELEM_INT16:    return (RTYPE) src->elem_int16;               \
	case VTM_ELEM_UINT16:   return (RTYPE) src->elem_uint16;              \
	case VTM_ELEM_INT32:    return (RTYPE) src->elem_int32;               \
	case VTM_ELEM_UINT32:   return (RTYPE) src->elem_uint32;              \
	case VTM_ELEM_INT64:    return (RTYPE) src->elem_int64;               \
	case VTM_ELEM_UINT64:   return (RTYPE) src->elem_uint64;              \
	case VTM_ELEM_BOOL:     return (RTYPE) (src->elem_bool) ? 1 : 0;      \
	case VTM_ELEM_CHAR:     return (RTYPE) src->elem_char;                \
	case VTM_ELEM_SCHAR:    return (RTYPE) src->elem_schar;               \
	case VTM_ELEM_UCHAR:    return (RTYPE) src->elem_uchar;               \
	case VTM_ELEM_SHORT:    return (RTYPE) src->elem_short;               \
	case VTM_ELEM_USHORT:   return (RTYPE) src->elem_ushort;              \
	case VTM_ELEM_INT:      return (RTYPE) src->elem_int;                 \
	case VTM_ELEM_UINT:     return (RTYPE) src->elem_uint;                \
	case VTM_ELEM_LONG:     return (RTYPE) src->elem_long;                \
	case VTM_ELEM_ULONG:    return (RTYPE) src->elem_ulong;               \
	case VTM_ELEM_FLOAT:    return (RTYPE) src->elem_float;               \
	case VTM_ELEM_DOUBLE:   return (RTYPE) src->elem_double;              \
	case VTM_ELEM_STRING:   return (RTYPE) vtm_conv_str_int64((char*) src->elem_pointer); \
	case VTM_ELEM_POINTER:  return (RTYPE) (intptr_t) src->elem_pointer;  \
	default:                                                              \
		VTM_ABORT_NOT_SUPPORTED;                                          \
	}                                                                     \
	return 0;

#define VTM_ELEM_CONV_UNUMERIC(RTYPE)                                     \
	switch (type) {                                                       \
	case VTM_ELEM_INT8:     return (RTYPE) src->elem_int8;                \
	case VTM_ELEM_UINT8:    return (RTYPE) src->elem_uint8;               \
	case VTM_ELEM_INT16:    return (RTYPE) src->elem_int16;               \
	case VTM_ELEM_UINT16:   return (RTYPE) src->elem_uint16;              \
	case VTM_ELEM_INT32:    return (RTYPE) src->elem_int32;               \
	case VTM_ELEM_UINT32:   return (RTYPE) src->elem_uint32;              \
	case VTM_ELEM_INT64:    return (RTYPE) src->elem_int64;               \
	case VTM_ELEM_UINT64:   return (RTYPE) src->elem_uint64;              \
	case VTM_ELEM_BOOL:     return (RTYPE) (src->elem_bool) ? 1 : 0;      \
	case VTM_ELEM_CHAR:     return (RTYPE) src->elem_char;                \
	case VTM_ELEM_SCHAR:    return (RTYPE) src->elem_schar;               \
	case VTM_ELEM_UCHAR:    return (RTYPE) src->elem_uchar;               \
	case VTM_ELEM_SHORT:    return (RTYPE) src->elem_short;               \
	case VTM_ELEM_USHORT:   return (RTYPE) src->elem_ushort;              \
	case VTM_ELEM_INT:      return (RTYPE) src->elem_int;                 \
	case VTM_ELEM_UINT:     return (RTYPE) src->elem_uint;                \
	case VTM_ELEM_LONG:     return (RTYPE) src->elem_long;                \
	case VTM_ELEM_ULONG:    return (RTYPE) src->elem_ulong;               \
	case VTM_ELEM_FLOAT:    return (RTYPE) src->elem_float;               \
	case VTM_ELEM_DOUBLE:   return (RTYPE) src->elem_double;              \
	case VTM_ELEM_STRING:   return (RTYPE) vtm_conv_str_uint64((char*) src->elem_pointer); \
	case VTM_ELEM_POINTER:  return (RTYPE) (uintptr_t) src->elem_pointer; \
	default:                                                              \
		VTM_ABORT_NOT_SUPPORTED;                                          \
	}                                                                     \
	return 0;

int8_t vtm_elem_as_int8(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_NUMERIC(int8_t);
}

uint8_t vtm_elem_as_uint8(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_UNUMERIC(uint8_t);
}

int16_t vtm_elem_as_int16(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_NUMERIC(int16_t);
}

uint16_t vtm_elem_as_uint16(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_UNUMERIC(uint16_t);
}

int32_t vtm_elem_as_int32(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_NUMERIC(int32_t);
}

uint32_t vtm_elem_as_uint32(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_UNUMERIC(uint32_t);
}

int64_t vtm_elem_as_int64(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_NUMERIC(int64_t);
}

uint64_t vtm_elem_as_uint64(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_UNUMERIC(uint64_t);
}

bool vtm_elem_as_bool(enum vtm_elem_type type, const union vtm_elem *src)
{
	switch (type) {
		case VTM_ELEM_INT8:     return src->elem_int8 ? true : false;
		case VTM_ELEM_UINT8:    return src->elem_uint8 ? true : false;
		case VTM_ELEM_INT16:    return src->elem_int16 ? true : false;
		case VTM_ELEM_UINT16:   return src->elem_uint16 ? true : false;
		case VTM_ELEM_INT32:    return src->elem_int32 ? true : false;
		case VTM_ELEM_UINT32:   return src->elem_uint32 ? true : false;
		case VTM_ELEM_INT64:    return src->elem_int64 ? true : false;
		case VTM_ELEM_UINT64:   return src->elem_uint64 ? true : false;
		case VTM_ELEM_BOOL:     return src->elem_bool;
		case VTM_ELEM_CHAR:     return src->elem_char ? true : false;
		case VTM_ELEM_UCHAR:    return src->elem_uchar ? true : false;
		case VTM_ELEM_SHORT:    return src->elem_short ? true : false;
		case VTM_ELEM_USHORT:   return src->elem_ushort ? true : false;
		case VTM_ELEM_INT:      return src->elem_int ? true : false;
		case VTM_ELEM_UINT:     return src->elem_uint ? true : false;
		case VTM_ELEM_LONG:     return src->elem_long ? true : false;
		case VTM_ELEM_ULONG:    return src->elem_ulong ? true : false;
		case VTM_ELEM_FLOAT:    return src->elem_float ? true : false;
		case VTM_ELEM_DOUBLE:   return src->elem_double ? true : false;
		case VTM_ELEM_STRING:   return strcmp(VTM_ELEM_BOOL_TRUE, (char*) src->elem_pointer) == 0 ? true : false;
		case VTM_ELEM_POINTER:  return false;
		default:
			VTM_ABORT_NOT_SUPPORTED;
	}
	return 0;
}

char vtm_elem_as_char(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_NUMERIC(char);
}

signed char vtm_elem_as_schar(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_NUMERIC(signed char);
}

unsigned char vtm_elem_as_uchar(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_UNUMERIC(unsigned char);
}

short vtm_elem_as_short(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_NUMERIC(short);
}

unsigned short vtm_elem_as_ushort(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_UNUMERIC(unsigned short);
}

int vtm_elem_as_int(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_NUMERIC(int);
}

unsigned int vtm_elem_as_uint(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_UNUMERIC(unsigned int);
}

long vtm_elem_as_long(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_NUMERIC(long);
}

unsigned long vtm_elem_as_ulong(enum vtm_elem_type type, const union vtm_elem *src)
{
	VTM_ELEM_CONV_UNUMERIC(unsigned long);
}

float vtm_elem_as_float(enum vtm_elem_type type, const union vtm_elem *src)
{
	if (type == VTM_ELEM_FLOAT)
		return src->elem_float;

	return (float) vtm_elem_as_double(type, src);
}

double vtm_elem_as_double(enum vtm_elem_type type, const union vtm_elem *src)
{
	switch (type) {
		case VTM_ELEM_INT8:     return (double) src->elem_int8;
		case VTM_ELEM_UINT8:    return (double) src->elem_uint8;
		case VTM_ELEM_INT16:    return (double) src->elem_int16;
		case VTM_ELEM_UINT16:   return (double) src->elem_uint16;
		case VTM_ELEM_INT32:    return (double) src->elem_int32;
		case VTM_ELEM_UINT32:   return (double) src->elem_uint32;
		case VTM_ELEM_INT64:    return (double) src->elem_int64;
		case VTM_ELEM_UINT64:   return (double) src->elem_uint64;
		case VTM_ELEM_BOOL:     return (double) src->elem_bool ? 1 : 0;
		case VTM_ELEM_CHAR:     return (double) src->elem_char;
		case VTM_ELEM_SCHAR:    return (double) src->elem_schar;
		case VTM_ELEM_UCHAR:    return (double) src->elem_uchar;
		case VTM_ELEM_SHORT:    return (double) src->elem_short;
		case VTM_ELEM_USHORT:   return (double) src->elem_ushort;
		case VTM_ELEM_INT:      return (double) src->elem_int;
		case VTM_ELEM_UINT:     return (double) src->elem_uint;
		case VTM_ELEM_LONG:     return (double) src->elem_long;
		case VTM_ELEM_ULONG:    return (double) src->elem_ulong;
		case VTM_ELEM_FLOAT:    return (double) src->elem_float;
		case VTM_ELEM_DOUBLE:   return (double) src->elem_double;
		case VTM_ELEM_STRING:   return vtm_conv_str_double((char*) src->elem_pointer);
		case VTM_ELEM_POINTER:  return 0;
		default:
			VTM_ABORT_NOT_SUPPORTED;
	}
}

char* vtm_elem_as_str(enum vtm_elem_type type, const union vtm_elem *src)
{
	switch (type) {
		case VTM_ELEM_INT8:     return vtm_conv_int64_str(src->elem_int8);
		case VTM_ELEM_UINT8:    return vtm_conv_uint64_str(src->elem_uint8);
		case VTM_ELEM_INT16:    return vtm_conv_int64_str(src->elem_int16);
		case VTM_ELEM_UINT16:   return vtm_conv_uint64_str(src->elem_uint16);
		case VTM_ELEM_INT32:    return vtm_conv_int64_str(src->elem_int32);
		case VTM_ELEM_UINT32:   return vtm_conv_uint64_str(src->elem_uint32);
		case VTM_ELEM_INT64:    return vtm_conv_int64_str(src->elem_int64);
		case VTM_ELEM_UINT64:   return vtm_conv_uint64_str(src->elem_uint64);
		case VTM_ELEM_BOOL:     return src->elem_bool ? vtm_str_copy(VTM_ELEM_BOOL_TRUE) : vtm_str_copy(VTM_ELEM_BOOL_FALSE);
		case VTM_ELEM_CHAR:     return vtm_conv_int64_str(src->elem_char);
		case VTM_ELEM_SCHAR:    return vtm_conv_int64_str(src->elem_schar);
		case VTM_ELEM_UCHAR:    return vtm_conv_uint64_str(src->elem_uchar);
		case VTM_ELEM_SHORT:    return vtm_conv_int64_str(src->elem_short);
		case VTM_ELEM_USHORT:   return vtm_conv_uint64_str(src->elem_ushort);
		case VTM_ELEM_INT:      return vtm_conv_int64_str(src->elem_int);
		case VTM_ELEM_UINT:     return vtm_conv_uint64_str(src->elem_uint);
		case VTM_ELEM_LONG:     return vtm_conv_int64_str(src->elem_long);
		case VTM_ELEM_ULONG:    return vtm_conv_uint64_str(src->elem_ulong);
		case VTM_ELEM_FLOAT:    return vtm_conv_double_str(src->elem_float);
		case VTM_ELEM_DOUBLE:   return vtm_conv_double_str(src->elem_double);
		case VTM_ELEM_STRING:   return (char*) src->elem_pointer;
		case VTM_ELEM_POINTER:  return vtm_conv_uint64_str((uintptr_t) src->elem_pointer);
		default:
			VTM_ABORT_NOT_SUPPORTED;
	}
	return NULL;
}

void* vtm_elem_as_ptr(enum vtm_elem_type type, const union vtm_elem *src)
{
	if (type == VTM_ELEM_POINTER)
		return src->elem_pointer;

	return NULL;
}

/* compare functions */
bool vtm_elem_cmp_int8(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_int8 == e2->elem_int8;
}

bool vtm_elem_cmp_uint8(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_uint8 == e2->elem_uint8;
}

bool vtm_elem_cmp_int16(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_int16 == e2->elem_int16;
}

bool vtm_elem_cmp_uint16(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_uint16 == e2->elem_uint16;
}

bool vtm_elem_cmp_int32(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_int32 == e2->elem_int32;
}

bool vtm_elem_cmp_uint32(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_uint32 == e2->elem_uint32;
}

bool vtm_elem_cmp_int64(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_int64 == e2->elem_int64;
}

bool vtm_elem_cmp_uint64(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_uint64 == e2->elem_uint64;
}

bool vtm_elem_cmp_bool(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_bool == e2->elem_bool;
}

bool vtm_elem_cmp_char(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_char == e2->elem_char;
}

bool vtm_elem_cmp_schar(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_schar == e2->elem_schar;
}

bool vtm_elem_cmp_uchar(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_uchar == e2->elem_uchar;
}

bool vtm_elem_cmp_short(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_short == e2->elem_short;
}

bool vtm_elem_cmp_ushort(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_ushort == e2->elem_ushort;
}

bool vtm_elem_cmp_int(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_int == e2->elem_int;
}

bool vtm_elem_cmp_uint(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_uint == e2->elem_uint;
}

bool vtm_elem_cmp_long(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_long == e2->elem_long;
}

bool vtm_elem_cmp_ulong(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_ulong == e2->elem_ulong;
}

bool vtm_elem_cmp_float(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_float == e2->elem_float;
}

bool vtm_elem_cmp_double(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_double == e2->elem_double;
}

bool vtm_elem_cmp_str(union vtm_elem *e1, union vtm_elem *e2)
{
	return strcmp(e1->elem_pointer, e2->elem_pointer) == 0;
}

bool vtm_elem_cmp_strcase(union vtm_elem *e1, union vtm_elem *e2)
{
	return vtm_str_casecmp(e1->elem_pointer, e2->elem_pointer) == 0;
}

bool vtm_elem_cmp_ptr(union vtm_elem *e1, union vtm_elem *e2)
{
	return e1->elem_pointer == e2->elem_pointer;
}

vtm_elem_cmp_fn vtm_elem_get_cmp_fn(enum vtm_elem_type type)
{
	switch (type) {
		case VTM_ELEM_NULL:     return NULL;
		case VTM_ELEM_INT8:     return vtm_elem_cmp_int8;
		case VTM_ELEM_UINT8:    return vtm_elem_cmp_uint8;
		case VTM_ELEM_INT16:    return vtm_elem_cmp_int16;
		case VTM_ELEM_UINT16:   return vtm_elem_cmp_uint16;
		case VTM_ELEM_INT32:    return vtm_elem_cmp_int32;
		case VTM_ELEM_UINT32:   return vtm_elem_cmp_uint32;
		case VTM_ELEM_INT64:    return vtm_elem_cmp_int64;
		case VTM_ELEM_UINT64:   return vtm_elem_cmp_uint64;
		case VTM_ELEM_BOOL:     return vtm_elem_cmp_bool;
		case VTM_ELEM_CHAR:     return vtm_elem_cmp_char;
		case VTM_ELEM_SCHAR:    return vtm_elem_cmp_schar;
		case VTM_ELEM_UCHAR:    return vtm_elem_cmp_uchar;
		case VTM_ELEM_SHORT:    return vtm_elem_cmp_short;
		case VTM_ELEM_USHORT:   return vtm_elem_cmp_ushort;
		case VTM_ELEM_INT:      return vtm_elem_cmp_int;
		case VTM_ELEM_UINT:     return vtm_elem_cmp_uint;
		case VTM_ELEM_LONG:     return vtm_elem_cmp_long;
		case VTM_ELEM_ULONG:    return vtm_elem_cmp_ulong;
		case VTM_ELEM_FLOAT:    return vtm_elem_cmp_float;
		case VTM_ELEM_DOUBLE:   return vtm_elem_cmp_double;
		case VTM_ELEM_STRING:   return vtm_elem_cmp_str;
		case VTM_ELEM_POINTER:  return vtm_elem_cmp_ptr;
	}

	VTM_ABORT_NOT_REACHABLE;
	return NULL;
}
