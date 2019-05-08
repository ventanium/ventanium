/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "list.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vtm/core/error.h>

/* maximum number of elements a list can hold */
#define MAX_LIST_SIZE SIZE_MAX

/* reads sizeof(TYPE) bytes from list array into given type variable
 * and returns that */
#define VTM_LIST_READ(TYPE)                                \
	TYPE result;                                           \
	result = ((TYPE*) li->array)[index];                   \
	return result;

/* writes sizeof(TYPE) bytes to list array into memory at index */
#define VTM_LIST_WRITE(TYPE)                               \
	((TYPE*)li->array)[li->used++] = va_arg(args, TYPE);

/* writes sizeof(TYPE) bytes to list array into memory at index,
 * the result from va_arg() is casted because it was promoted first */
#define VTM_LIST_WRITE_CASTED(TYPE, VATYPE)                \
	((TYPE*)li->array)[li->used++] = (TYPE) va_arg(args, VATYPE);

#define VTM_LIST_WRITE_VAR(TYPE, ELTYPE)                   \
	((TYPE*)li->array)[li->used++] = var.data.elem_ ## ELTYPE

struct vtm_list
{
	enum vtm_elem_type type;
	vtm_list_free_func fn_free;
	void *array;
	size_t elem_size;
	size_t used;
	size_t size;
};

/* forward declarations */
static int vtm_list_grow(vtm_list* li);

vtm_list* vtm_list_new(enum vtm_elem_type type, size_t init_size)
{
	vtm_list *li;

	if (type == VTM_ELEM_NULL) {
		vtm_err_set(VTM_E_INVALID_ARG);
		return NULL;
	}

	li = malloc(sizeof(vtm_list));
	if (!li) {
		vtm_err_oom();
		return NULL;
	}

	li->elem_size = vtm_elem_size(type);
	li->array = malloc(init_size * li->elem_size);
	if (!li->array) {
		vtm_err_oom();
		free(li);
		return NULL;
	}

	li->type = type;
	li->fn_free = NULL;
	li->used = 0;
	li->size = init_size;

	return li;
}

void vtm_list_free(vtm_list *li)
{
	if (!li)
		return;

	vtm_list_clear(li);
	free(li->array);
	free(li);
}

void vtm_list_clear(vtm_list *li)
{
	size_t i;
	void *val;

	if (li->fn_free) {
		for (i=0; i < li->used; i++) {
			val = vtm_list_get_pointer(li, i);
			li->fn_free(val);
		}
	}

	li->used = 0;
}

size_t vtm_list_size(vtm_list *li)
{
	return li->used;
}

int vtm_list_add(vtm_list *li, struct vtm_variant var)
{
	int rc;

	if (var.type != li->type)
		return VTM_E_INVALID_ARG;

	if (li->used == li->size) {
		rc = vtm_list_grow(li);
		if (rc != VTM_OK)
			return rc;
	}

	switch (li->type) {
		case VTM_ELEM_NULL:     return VTM_E_INVALID_STATE;

		case VTM_ELEM_INT8:     VTM_LIST_WRITE_VAR(int8_t, int8); break;
		case VTM_ELEM_UINT8:    VTM_LIST_WRITE_VAR(uint8_t, uint8); break;

		case VTM_ELEM_INT16:    VTM_LIST_WRITE_VAR(int16_t, int16); break;
		case VTM_ELEM_UINT16:   VTM_LIST_WRITE_VAR(uint16_t, uint16); break;

		case VTM_ELEM_INT32:    VTM_LIST_WRITE_VAR(int32_t, int32); break;
		case VTM_ELEM_UINT32:   VTM_LIST_WRITE_VAR(uint32_t, uint32); break;

		case VTM_ELEM_INT64:    VTM_LIST_WRITE_VAR(int64_t, int64); break;
		case VTM_ELEM_UINT64:   VTM_LIST_WRITE_VAR(uint64_t, uint64); break;

		case VTM_ELEM_BOOL:     VTM_LIST_WRITE_VAR(bool, bool); break;

		case VTM_ELEM_CHAR:     VTM_LIST_WRITE_VAR(char, char); break;
		case VTM_ELEM_SCHAR:    VTM_LIST_WRITE_VAR(signed char, schar); break;
		case VTM_ELEM_UCHAR:    VTM_LIST_WRITE_VAR(unsigned char, uchar); break;

		case VTM_ELEM_SHORT:    VTM_LIST_WRITE_VAR(short, short); break;
		case VTM_ELEM_USHORT:   VTM_LIST_WRITE_VAR(unsigned short, ushort); break;

		case VTM_ELEM_INT:      VTM_LIST_WRITE_VAR(int, int); break;
		case VTM_ELEM_UINT:     VTM_LIST_WRITE_VAR(unsigned int, uint); break;

		case VTM_ELEM_LONG:     VTM_LIST_WRITE_VAR(long, long); break;
		case VTM_ELEM_ULONG:    VTM_LIST_WRITE_VAR(unsigned long, ulong); break;

		case VTM_ELEM_FLOAT:    VTM_LIST_WRITE_VAR(float, float); break;
		case VTM_ELEM_DOUBLE:   VTM_LIST_WRITE_VAR(double, double); break;

		case VTM_ELEM_STRING:
		case VTM_ELEM_BLOB:
		case VTM_ELEM_POINTER:  VTM_LIST_WRITE_VAR(void*, pointer); break;
	}

	return VTM_OK;
}

int vtm_list_add_va(vtm_list *li, ...)
{
	int rc;
	va_list args;

	if (li->used == li->size) {
		rc = vtm_list_grow(li);
		if (rc != VTM_OK)
			return rc;
	}

	rc = VTM_OK;
	va_start(args, li);

	switch (li->type) {
		case VTM_ELEM_NULL:     rc = VTM_E_INVALID_STATE; break;

		case VTM_ELEM_INT8:     VTM_LIST_WRITE_CASTED(int8_t, int); break;
		case VTM_ELEM_UINT8:    VTM_LIST_WRITE_CASTED(uint8_t, unsigned int); break;

		case VTM_ELEM_INT16:    VTM_LIST_WRITE_CASTED(int16_t, int); break;
		case VTM_ELEM_UINT16:   VTM_LIST_WRITE_CASTED(uint16_t, unsigned int); break;

		case VTM_ELEM_INT32:    VTM_LIST_WRITE(int32_t); break;
		case VTM_ELEM_UINT32:   VTM_LIST_WRITE(uint32_t); break;

		case VTM_ELEM_INT64:    VTM_LIST_WRITE(int64_t); break;
		case VTM_ELEM_UINT64:   VTM_LIST_WRITE(uint64_t); break;

		case VTM_ELEM_BOOL:     VTM_LIST_WRITE_CASTED(bool, int); break;

		case VTM_ELEM_CHAR:     VTM_LIST_WRITE_CASTED(char, int); break;
		case VTM_ELEM_SCHAR:    VTM_LIST_WRITE_CASTED(signed char, signed int); break;
		case VTM_ELEM_UCHAR:    VTM_LIST_WRITE_CASTED(unsigned char, unsigned int); break;

		case VTM_ELEM_SHORT:    VTM_LIST_WRITE_CASTED(short, int); break;
		case VTM_ELEM_USHORT:   VTM_LIST_WRITE_CASTED(unsigned short, unsigned int); break;

		case VTM_ELEM_INT:      VTM_LIST_WRITE(int); break;
		case VTM_ELEM_UINT:     VTM_LIST_WRITE(unsigned int); break;

		case VTM_ELEM_LONG:     VTM_LIST_WRITE(long); break;
		case VTM_ELEM_ULONG:    VTM_LIST_WRITE(unsigned long); break;

		case VTM_ELEM_FLOAT:    VTM_LIST_WRITE_CASTED(float, double); break;
		case VTM_ELEM_DOUBLE:   VTM_LIST_WRITE(double); break;

		case VTM_ELEM_STRING:   VTM_LIST_WRITE(char*); break;
		case VTM_ELEM_BLOB:     VTM_LIST_WRITE(void*); break;
		case VTM_ELEM_POINTER:  VTM_LIST_WRITE(void*); break;
	}

	va_end(args);

	return rc;
}

int vtm_list_remove(vtm_list *li, size_t index)
{
	char *src;
	char *dst;

	if (index >= li->used)
		return VTM_E_OUT_OF_RANGE;

	dst = ((char*) li->array) + index * li->elem_size;
	src = dst + li->elem_size;
	memmove(dst, src, (li->used - (index+1)) * li->elem_size);
	li->used--;

	return VTM_OK;
}

void vtm_list_set_free_func(vtm_list *li, vtm_list_free_func fn)
{
	li->fn_free = fn;
}

static int vtm_list_grow(vtm_list *li)
{
	size_t old;

	if (li->size == MAX_LIST_SIZE)
		return VTM_E_MAX_REACHED;

	old = li->size;
	li->size *= 2;
	if (li->size < old)
		li->size = SIZE_MAX;

    li->array = realloc(li->array, li->size * li->elem_size);
    if (!li->array) {
		vtm_err_oom();
		return vtm_err_get_code();
	}

	return VTM_OK;
}

int8_t vtm_list_get_int8(vtm_list *li, size_t index)
{
	VTM_LIST_READ(int8_t);
}

uint8_t vtm_list_get_uint8(vtm_list *li, size_t index)
{
	VTM_LIST_READ(uint8_t);
}

int16_t vtm_list_get_int16(vtm_list *li, size_t index)
{
	VTM_LIST_READ(int16_t);
}

uint16_t vtm_list_get_uint16(vtm_list *li, size_t index)
{
	VTM_LIST_READ(uint16_t);
}

int32_t vtm_list_get_int32(vtm_list *li, size_t index)
{
	VTM_LIST_READ(int32_t);
}

uint32_t vtm_list_get_uint32(vtm_list *li, size_t index)
{
	VTM_LIST_READ(uint32_t);
}

int64_t vtm_list_get_int64(vtm_list *li, size_t index)
{
	VTM_LIST_READ(int64_t);
}

uint64_t vtm_list_get_uint64(vtm_list *li, size_t index)
{
	VTM_LIST_READ(uint64_t);
}

bool vtm_list_get_bool(vtm_list *li, size_t index)
{
	VTM_LIST_READ(bool);
}

char vtm_list_get_char(vtm_list *li, size_t index)
{
	VTM_LIST_READ(char);
}

signed char vtm_list_get_schar(vtm_list *li, size_t index)
{
	VTM_LIST_READ(signed char);
}

unsigned char vtm_list_get_uchar(vtm_list *li, size_t index)
{
	VTM_LIST_READ(unsigned char);
}

short vtm_list_get_short(vtm_list *li, size_t index)
{
	VTM_LIST_READ(short);
}

unsigned short vtm_list_get_ushort(vtm_list *li, size_t index)
{
	VTM_LIST_READ(unsigned short);
}

int vtm_list_get_int(vtm_list *li, size_t index)
{
	VTM_LIST_READ(int);
}

unsigned int vtm_list_get_uint(vtm_list *li, size_t index)
{
	VTM_LIST_READ(unsigned int);
}

long vtm_list_get_long(vtm_list *li, size_t index)
{
	VTM_LIST_READ(long);
}

unsigned long vtm_list_get_ulong(vtm_list *li, size_t index)
{
	VTM_LIST_READ(unsigned long);
}

float vtm_list_get_float(vtm_list *li, size_t index)
{
	VTM_LIST_READ(float);
}

double vtm_list_get_double(vtm_list *li, size_t index)
{
	VTM_LIST_READ(double);
}

void* vtm_list_get_pointer(vtm_list *li, size_t index)
{
	VTM_LIST_READ(void*);
}
