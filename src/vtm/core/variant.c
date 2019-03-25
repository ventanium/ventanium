/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "variant.h"

#include <stdlib.h> /* free() */

#define VTM_VARIANT_AS(RTYPE) \
	return vtm_elem_as_ ## RTYPE(var->type, &(var->data))

#define VTM_VARIANT_FROM(UNIONELEM, ENUMTYPE) \
	struct vtm_variant result;                \
	result.data.elem_ ## UNIONELEM = val;     \
	result.type = ENUMTYPE;                   \
	result.str = NULL;                        \
	return result;

void vtm_variant_init(struct vtm_variant *var)
{
	var->str = NULL;
}

void vtm_variant_release(struct vtm_variant *var)
{
	if (var->str && var->type != VTM_ELEM_STRING)
		free((char*) var->str);
}

int8_t vtm_variant_as_int8(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(int8);
}

uint8_t vtm_variant_as_uint8(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(uint8);
}

int16_t vtm_variant_as_int16(const struct  vtm_variant *var)
{
	VTM_VARIANT_AS(int16);
}

uint16_t vtm_variant_as_uint16(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(uint16);
}

int32_t vtm_variant_as_int32(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(int32);
}

uint32_t vtm_variant_as_uint32(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(uint32);
}

int64_t vtm_variant_as_int64(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(int64);
}

uint64_t vtm_variant_as_uint64(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(uint64);
}

bool vtm_variant_as_bool(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(bool);
}

char vtm_variant_as_char(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(char);
}

signed char vtm_variant_as_schar(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(schar);
}

unsigned char vtm_variant_as_uchar(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(uchar);
}

short vtm_variant_as_short(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(short);
}

unsigned short vtm_variant_as_ushort(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(ushort);
}

int vtm_variant_as_int(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(int);
}

unsigned int vtm_variant_as_uint(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(uint);
}

long vtm_variant_as_long(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(long);
}

unsigned long vtm_variant_as_ulong(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(ulong);
}

float vtm_variant_as_float(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(float);
}

double vtm_variant_as_double(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(double);
}

const char* vtm_variant_as_str(struct vtm_variant *var)
{
	if (!var->str)
		var->str = vtm_elem_as_str(var->type, &(var->data));

	return var->str;
}

const void* vtm_variant_as_blob(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(blob);
}

void* vtm_variant_as_ptr(const struct vtm_variant *var)
{
	VTM_VARIANT_AS(ptr);
}

struct vtm_variant vtm_variant_from_null(void)
{
	struct vtm_variant var;

	var.type = VTM_ELEM_NULL;
	var.data.elem_pointer = NULL;
	var.str = NULL;

	return var;
}

struct vtm_variant vtm_variant_from_int8(int8_t val)
{
	VTM_VARIANT_FROM(int8, VTM_ELEM_INT8);
}

struct vtm_variant vtm_variant_from_uint8(uint8_t val)
{
	VTM_VARIANT_FROM(uint8, VTM_ELEM_UINT8);
}

struct vtm_variant vtm_variant_from_int16(int16_t val)
{
	VTM_VARIANT_FROM(int16, VTM_ELEM_INT16);
}

struct vtm_variant vtm_variant_from_uint16(uint16_t val)
{
	VTM_VARIANT_FROM(uint16, VTM_ELEM_UINT16);
}

struct vtm_variant vtm_variant_from_int32(int32_t val)
{
	VTM_VARIANT_FROM(int32, VTM_ELEM_INT32);
}

struct vtm_variant vtm_variant_from_uint32(uint32_t val)
{
	VTM_VARIANT_FROM(uint32, VTM_ELEM_UINT32);
}

struct vtm_variant vtm_variant_from_int64(int64_t val)
{
	VTM_VARIANT_FROM(int64, VTM_ELEM_INT64);
}

struct vtm_variant vtm_variant_from_uint64(uint64_t val)
{
	VTM_VARIANT_FROM(uint64, VTM_ELEM_UINT64);
}

struct vtm_variant vtm_variant_from_bool(bool val)
{
	VTM_VARIANT_FROM(bool, VTM_ELEM_BOOL);
}

struct vtm_variant vtm_variant_from_char(char val)
{
	VTM_VARIANT_FROM(char, VTM_ELEM_CHAR);
}

struct vtm_variant vtm_variant_from_schar(signed char val)
{
	VTM_VARIANT_FROM(schar, VTM_ELEM_SCHAR);
}

struct vtm_variant vtm_variant_from_uchar(unsigned char val)
{
	VTM_VARIANT_FROM(uchar, VTM_ELEM_UCHAR);
}

struct vtm_variant vtm_variant_from_short(short val)
{
	VTM_VARIANT_FROM(short, VTM_ELEM_SHORT);
}

struct vtm_variant vtm_variant_from_ushort(unsigned short val)
{
	VTM_VARIANT_FROM(ushort, VTM_ELEM_USHORT);
}

struct vtm_variant vtm_variant_from_int(int val)
{
	VTM_VARIANT_FROM(int, VTM_ELEM_INT);
}

struct vtm_variant vtm_variant_from_uint(unsigned int val)
{
	VTM_VARIANT_FROM(uint, VTM_ELEM_UINT);
}

struct vtm_variant vtm_variant_from_long(long val)
{
	VTM_VARIANT_FROM(long, VTM_ELEM_LONG);
}

struct vtm_variant vtm_variant_from_ulong(unsigned long val)
{
	VTM_VARIANT_FROM(ulong, VTM_ELEM_ULONG);
}

struct vtm_variant vtm_variant_from_float(float val)
{
	VTM_VARIANT_FROM(float, VTM_ELEM_FLOAT);
}

struct vtm_variant vtm_variant_from_double(double val)
{
	VTM_VARIANT_FROM(double, VTM_ELEM_DOUBLE);
}

struct vtm_variant vtm_variant_from_str(char *val)
{
	VTM_VARIANT_FROM(pointer, VTM_ELEM_STRING);
}

struct vtm_variant vtm_variant_from_blob(void *val)
{
	VTM_VARIANT_FROM(pointer, VTM_ELEM_BLOB);
}

struct vtm_variant vtm_variant_from_ptr(void *val)
{
	VTM_VARIANT_FROM(pointer, VTM_ELEM_POINTER);
}
