/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "hash.h"

#include <string.h> /* memcpy() */
#include <ctype.h> /* tolower() */
#include <vtm/core/error.h>
#include <vtm/core/blob.h>

#define VTM_HASH_INIT                     2503  /* prime */
#define VTM_HASH_MAGIC_32           0x54321bb7  /* prime */
#define VTM_HASH_MAGIC_64   0x4c13b5651eba82cb  /* prime */

uint32_t vtm_hash_str(const char *in)
{
	uint32_t hash;
	char c;

	hash = VTM_HASH_INIT;

	while ((c = *in++))
		hash = ((hash << 13) - hash) ^ c;

	return hash;
}

uint32_t vtm_hash_strcase(const char *in)
{
	uint32_t hash;
	char c;

	hash = VTM_HASH_INIT;

	while ((c = *in++))
		hash = ((hash << 13) - hash) ^ tolower(c);

	return hash;
}

uint32_t vtm_hash_mem(const void *in, size_t len)
{
	uint32_t hash;
	const char *p;
	size_t i;

	p = in;
	hash = VTM_HASH_INIT;

	for (i=0; i < len; i++)
		hash = ((hash << 13) - hash) ^ + p[i];

	return hash;
}

uint32_t vtm_hash_ptr(const void *in)
{
	return vtm_hash_unum(((uint64_t) (uintptr_t) in) * VTM_HASH_MAGIC_64);
}

uint32_t vtm_hash_num(int64_t in)
{
	return vtm_hash_unum((uint64_t) in);
}

uint32_t vtm_hash_unum(uint64_t in)
{
	uint32_t result;

	result  = in >> 32;
	result += in & 0xffffffff;
	result *= VTM_HASH_MAGIC_32;

	return result;
}

vtm_hash_elem_fn vtm_hash_elem_get_fn(enum vtm_elem_type type)
{
	switch (type) {
		case VTM_ELEM_NULL:     return NULL;
		case VTM_ELEM_INT8:     return vtm_hash_elem_int8;
		case VTM_ELEM_UINT8:    return vtm_hash_elem_uint8;
		case VTM_ELEM_INT16:    return vtm_hash_elem_int16;
		case VTM_ELEM_UINT16:   return vtm_hash_elem_uint16;
		case VTM_ELEM_INT32:    return vtm_hash_elem_int32;
		case VTM_ELEM_UINT32:   return vtm_hash_elem_uint32;
		case VTM_ELEM_INT64:    return vtm_hash_elem_int64;
		case VTM_ELEM_UINT64:   return vtm_hash_elem_uint64;
		case VTM_ELEM_BOOL:     return vtm_hash_elem_bool;
		case VTM_ELEM_CHAR:     return vtm_hash_elem_char;
		case VTM_ELEM_SCHAR:    return vtm_hash_elem_schar;
		case VTM_ELEM_UCHAR:    return vtm_hash_elem_uchar;
		case VTM_ELEM_SHORT:    return vtm_hash_elem_short;
		case VTM_ELEM_USHORT:   return vtm_hash_elem_ushort;
		case VTM_ELEM_INT:      return vtm_hash_elem_int;
		case VTM_ELEM_UINT:     return vtm_hash_elem_uint;
		case VTM_ELEM_LONG:     return vtm_hash_elem_long;
		case VTM_ELEM_ULONG:    return vtm_hash_elem_ulong;
		case VTM_ELEM_FLOAT:    return vtm_hash_elem_float;
		case VTM_ELEM_DOUBLE:   return vtm_hash_elem_double;
		case VTM_ELEM_STRING:   return vtm_hash_elem_str;
		case VTM_ELEM_BLOB:     return vtm_hash_elem_blob;
		case VTM_ELEM_POINTER:  return vtm_hash_elem_ptr;
	}

	VTM_ABORT_NOT_REACHABLE;
	return NULL;
}

uint32_t vtm_hash_elem_int8(union vtm_elem *el)
{
	return vtm_hash_num(el->elem_int8);
}

uint32_t vtm_hash_elem_uint8(union vtm_elem *el)
{
	return vtm_hash_unum(el->elem_uint8);
}

uint32_t vtm_hash_elem_int16(union vtm_elem *el)
{
	return vtm_hash_num(el->elem_int16);
}

uint32_t vtm_hash_elem_uint16(union vtm_elem *el)
{
	return vtm_hash_unum(el->elem_uint16);
}

uint32_t vtm_hash_elem_int32(union vtm_elem *el)
{
	return vtm_hash_num(el->elem_int32);
}

uint32_t vtm_hash_elem_uint32(union vtm_elem *el)
{
	return vtm_hash_unum(el->elem_uint32);
}

uint32_t vtm_hash_elem_int64(union vtm_elem *el)
{
	return vtm_hash_num(el->elem_int64);
}

uint32_t vtm_hash_elem_uint64(union vtm_elem *el)
{
	return vtm_hash_unum(el->elem_uint64);
}

uint32_t vtm_hash_elem_bool(union vtm_elem *el)
{
	return vtm_hash_num(el->elem_bool);
}

uint32_t vtm_hash_elem_char(union vtm_elem *el)
{
	return vtm_hash_num(el->elem_char);
}

uint32_t vtm_hash_elem_schar(union vtm_elem *el)
{
	return vtm_hash_num(el->elem_schar);
}

uint32_t vtm_hash_elem_uchar(union vtm_elem *el)
{
	return vtm_hash_unum(el->elem_uchar);
}

uint32_t vtm_hash_elem_short(union vtm_elem *el)
{
	return vtm_hash_num(el->elem_short);
}

uint32_t vtm_hash_elem_ushort(union vtm_elem *el)
{
	return vtm_hash_unum(el->elem_ushort);
}

uint32_t vtm_hash_elem_int(union vtm_elem *el)
{
	return vtm_hash_num(el->elem_int);
}

uint32_t vtm_hash_elem_uint(union vtm_elem *el)
{
	return vtm_hash_unum(el->elem_uint);
}

uint32_t vtm_hash_elem_long(union vtm_elem *el)
{
	return vtm_hash_num(el->elem_long);
}

uint32_t vtm_hash_elem_ulong(union vtm_elem *el)
{
	return vtm_hash_unum(el->elem_ulong);
}

uint32_t vtm_hash_elem_float(union vtm_elem *el)
{
	return vtm_hash_mem(&(el->elem_float), sizeof(float));
}

uint32_t vtm_hash_elem_double(union vtm_elem *el)
{
	return vtm_hash_mem(&(el->elem_double), sizeof(double));
}

uint32_t vtm_hash_elem_str(union vtm_elem *el)
{
	return vtm_hash_str(el->elem_pointer);
}

uint32_t vtm_hash_elem_strcase(union vtm_elem *el)
{
	return vtm_hash_strcase(el->elem_pointer);
}

uint32_t vtm_hash_elem_blob(union vtm_elem *el)
{
	return vtm_hash_mem(el->elem_pointer, vtm_blob_size(el->elem_pointer));
}

uint32_t vtm_hash_elem_ptr(union vtm_elem *el)
{
	return vtm_hash_ptr(el->elem_pointer);
}
