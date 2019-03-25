/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "dataset.h"

#include <stdlib.h> /* malloc() */

#include <vtm/core/blob.h>
#include <vtm/core/elem.h>
#include <vtm/core/error.h>
#include <vtm/core/flag.h>
#include <vtm/core/hash.h>
#include <vtm/core/string.h>

#define VTM_DATASET_GET(RTYPE, NVL)                            \
	do {                                                       \
		struct vtm_variant *var;                               \
		var = vtm_dataset_get_variant(ds, name);               \
		if (var)                                               \
			return vtm_variant_as_ ## RTYPE(var);              \
		return NVL;                                            \
	} while (0)

#define VTM_DATASET_GET_NUMERIC(RTYPE) VTM_DATASET_GET(RTYPE, 0)

#define VTM_DATASET_SETX(VALTYPE, VALUE, FREE_VAL_PTR)         \
	do {                                                       \
		bool static_key;                                       \
		struct vtm_dataset_node *node;                         \
		node = malloc(sizeof(*node));                          \
		if (!node)                                             \
			vtm_err_oom();                                     \
		static_key = vtm_flag_is_set(ds->hints,                \
			VTM_DS_HINT_STATIC_NAMES);                         \
		node->key = static_key ? VTM_V_STR((char*) name) :     \
			VTM_V_STR(vtm_str_copy(name));                     \
		node->value = VALTYPE(VALUE);                          \
		node->next = NULL;                                     \
		node->free_key_ptr = !static_key;                      \
		node->free_val_ptr = FREE_VAL_PTR;                     \
		vtm_dataset_put_node(ds, node);                        \
	} while (0)

#define VTM_DATASET_SET(VALTYPE)	VTM_DATASET_SETX(VALTYPE, val, false)

struct vtm_dataset_node
{
	struct vtm_variant key;
	struct vtm_variant value;
	struct vtm_dataset_node *next;
	bool free_key_ptr : 1;
	bool free_val_ptr : 1;
};

/* forward declaration */
void vtm_dataset_free_node(struct vtm_dataset_node *node);

void vtm_dataset_init(vtm_dataset *ds, unsigned int hints)
{
	ds->hints = hints;
	VTM_SMAP_INIT(ds->vars, VTM_ELEM_STRING, vtm_dataset_free_node);

	if (hints & VTM_DS_HINT_IGNORE_CASE) {
		VTM_SMAP_SET_KEY_FN(ds->vars,
			vtm_hash_elem_strcase,
			vtm_elem_cmp_strcase);
	}
}

void vtm_dataset_release(vtm_dataset *ds)
{
	vtm_dataset_clear(ds);
}

vtm_dataset* vtm_dataset_new(void)
{
	return vtm_dataset_newh(VTM_DS_HINT_DEFAULT);
}

vtm_dataset* vtm_dataset_newh(unsigned int hints)
{
	vtm_dataset *ds;

	ds = malloc(sizeof(vtm_dataset));
	if (!ds) {
		vtm_err_oom();
		return NULL;
	}

	vtm_dataset_init(ds, hints);

	return ds;
}

void vtm_dataset_free(vtm_dataset *ds)
{
	if (!ds)
		return;

	vtm_dataset_release(ds);
	free(ds);
}

void vtm_dataset_clear(vtm_dataset *ds)
{
	VTM_SMAP_CLEAR(ds->vars, struct vtm_dataset_node);
}

void vtm_dataset_free_node(struct vtm_dataset_node *node)
{
	/* free value string representation */
	vtm_variant_release(&node->value);

	/* free key=fieldname str copy */
	if (node->free_key_ptr)
		free(node->key.data.elem_pointer);

	/* free val pointer? */
	if (node->free_val_ptr) {
		switch (node->value.type) {
			case VTM_ELEM_BLOB:
				vtm_blob_free(node->value.data.elem_pointer);
				break;

			case VTM_ELEM_STRING:
			case VTM_ELEM_POINTER:
				free(node->value.data.elem_pointer);
				break;

			default:
				break;
		}
	}
}

static void vtm_dataset_put_node(vtm_dataset *ds, struct vtm_dataset_node *node)
{
	VTM_SMAP_PUT(ds->vars, struct vtm_dataset_node, node);
}

static struct vtm_dataset_node* vtm_dataset_get_node(vtm_dataset *ds, const char *name)
{
	struct vtm_dataset_node *node;
	struct vtm_variant key;

	node = NULL;
	key.type = VTM_ELEM_STRING;
	key.data.elem_pointer = (char*) name;
	VTM_SMAP_GET(ds->vars, struct vtm_dataset_node, &key, node);

	return node;
}

vtm_list* vtm_dataset_entryset(vtm_dataset *ds)
{
	size_t entries;
	size_t bc;
	struct vtm_dataset_node *node;
	vtm_list *result;

	if (!ds)
		return NULL;

	bc = 0;
	entries = 0;
	VTM_SMAP_GET_SIZE(ds->vars, bc, node, entries);

	result = vtm_list_new(VTM_ELEM_POINTER, entries);
	if (!result) {
		vtm_err_oom();
		return NULL;
	}

	vtm_list_set_free_func(result, free);

	bc = 0;
	VTM_SMAP_FOR_EACH(ds->vars, bc, node) {
		struct vtm_dataset_entry *ds_entry;
		ds_entry = malloc(sizeof(*ds_entry));
		if (!ds_entry) {
			vtm_err_oom();
			goto err;
		}

		ds_entry->name = node->key.data.elem_pointer;
		ds_entry->var = &(node->value);
		vtm_list_add_va(result, ds_entry);
	}

	return result;

err:
	vtm_list_free(result);
	return NULL;
}

bool vtm_dataset_contains(vtm_dataset *ds, const char *name)
{
	struct vtm_dataset_node* node;

	node = vtm_dataset_get_node(ds, name);

	return node != NULL;
}

int vtm_dataset_remove(vtm_dataset *ds, const char *name)
{
	struct vtm_variant key;
	bool removed;

	key = VTM_V_STR((char*) name);
	VTM_SMAP_REMOVE(ds->vars, struct vtm_dataset_node, &key, removed);

	return removed ? VTM_OK : VTM_E_NOT_FOUND;
}

struct vtm_variant* vtm_dataset_get_variant(vtm_dataset *ds, const char *name)
{
	struct vtm_dataset_node *node;

	node = vtm_dataset_get_node(ds, name);
	if (node)
		return &(node->value);

	return NULL;
}

void vtm_dataset_set_variant(vtm_dataset *ds, const char *name, struct vtm_variant *var)
{
	bool static_key;
	struct vtm_dataset_node *node;

	node = malloc(sizeof(*node));
	if (!node)
		vtm_err_oom();

	static_key = vtm_flag_is_set(ds->hints, VTM_DS_HINT_STATIC_NAMES);

	node->key = static_key ? VTM_V_STR((char*) name) : VTM_V_STR(vtm_str_copy(name));
	node->value.type = var->type;
	node->value.data = var->data;
	node->value.str = NULL;
	node->free_key_ptr = !static_key;
	node->free_val_ptr = false;
	node->next = NULL;

	switch (var->type) {
		case VTM_ELEM_STRING:
			node->value.data.elem_pointer = vtm_str_copy(var->data.elem_pointer);
			node->free_val_ptr = true;
			break;

		case VTM_ELEM_BLOB:
			node->free_val_ptr = true;
			break;

		default:
			break;
	}

	vtm_dataset_put_node(ds, node);
}

int8_t vtm_dataset_get_int8(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(int8);
}

void vtm_dataset_set_int8(vtm_dataset *ds, const char *name, int8_t val)
{
	VTM_DATASET_SET(VTM_V_INT8);
}

uint8_t vtm_dataset_get_uint8(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(uint8);
}

void vtm_dataset_set_uint8(vtm_dataset *ds, const char *name, uint8_t val)
{
	VTM_DATASET_SET(VTM_V_UINT8);
}

int16_t vtm_dataset_get_int16(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(int16);
}

void vtm_dataset_set_int16(vtm_dataset *ds, const char *name, int16_t val)
{
	VTM_DATASET_SET(VTM_V_INT16);
}

uint16_t vtm_dataset_get_uint16(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(uint16);
}

void vtm_dataset_set_uint16(vtm_dataset *ds, const char *name, uint16_t val)
{
	VTM_DATASET_SET(VTM_V_UINT16);
}

int32_t vtm_dataset_get_int32(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(int32);
}

void vtm_dataset_set_int32(vtm_dataset *ds, const char *name, int32_t val)
{
	VTM_DATASET_SET(VTM_V_INT32);
}

uint32_t vtm_dataset_get_uint32(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(uint32);
}

void vtm_dataset_set_uint32(vtm_dataset *ds, const char *name, uint32_t val)
{
	VTM_DATASET_SET(VTM_V_UINT32);
}

int64_t vtm_dataset_get_int64(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(int64);
}

void vtm_dataset_set_int64(vtm_dataset *ds, const char *name, int64_t val)
{
	VTM_DATASET_SET(VTM_V_INT64);
}

uint64_t vtm_dataset_get_uint64(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(int64);
}

void vtm_dataset_set_uint64(vtm_dataset *ds, const char *name, uint64_t val)
{
	VTM_DATASET_SET(VTM_V_UINT64);
}

bool vtm_dataset_get_bool(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET(bool, false);
}

void vtm_dataset_set_bool(vtm_dataset *ds, const char *name, bool val)
{
	VTM_DATASET_SET(VTM_V_BOOL);
}

char vtm_dataset_get_char(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(char);
}

void vtm_dataset_set_char(vtm_dataset *ds, const char *name, char val)
{
	VTM_DATASET_SET(VTM_V_CHAR);
}

signed char vtm_dataset_get_schar(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(schar);
}

void vtm_dataset_set_schar(vtm_dataset *ds, const char *name, signed char val)
{
	VTM_DATASET_SET(VTM_V_SCHAR);
}

unsigned char vtm_dataset_get_uchar(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(uchar);
}

void vtm_dataset_set_uchar(vtm_dataset *ds, const char *name, unsigned char val)
{
	VTM_DATASET_SET(VTM_V_UCHAR);
}

short vtm_dataset_get_short(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(short);
}

void vtm_dataset_set_short(vtm_dataset *ds, const char *name, short val)
{
	VTM_DATASET_SET(VTM_V_SHORT);
}

unsigned short vtm_dataset_get_ushort(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(ushort);
}

void vtm_dataset_set_ushort(vtm_dataset *ds, const char *name, unsigned short val)
{
	VTM_DATASET_SET(VTM_V_USHORT);
}

int vtm_dataset_get_int(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(int);
}

void vtm_dataset_set_int(vtm_dataset *ds, const char *name, int val)
{
	VTM_DATASET_SET(VTM_V_INT);
}

unsigned int vtm_dataset_get_uint(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(uint);
}

void vtm_dataset_set_uint(vtm_dataset *ds, const char *name, unsigned int val)
{
	VTM_DATASET_SET(VTM_V_UINT);
}

long vtm_dataset_get_long(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(long);
}

void vtm_dataset_set_long(vtm_dataset *ds, const char *name, long val)
{
	VTM_DATASET_SET(VTM_V_LONG);
}

unsigned long vtm_dataset_get_ulong(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(ulong);
}

void vtm_dataset_set_ulong(vtm_dataset *ds, const char *name, unsigned long val)
{
	VTM_DATASET_SET(VTM_V_ULONG);
}

float vtm_dataset_get_float(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(float);
}

void vtm_dataset_set_float(vtm_dataset *ds, const char *name, float val)
{
	VTM_DATASET_SET(VTM_V_FLOAT);
}

double vtm_dataset_get_double(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET_NUMERIC(double);
}

void vtm_dataset_set_double(vtm_dataset *ds, const char *name, double val)
{
	VTM_DATASET_SET(VTM_V_DOUBLE);
}

const char* vtm_dataset_get_string(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET(str, NULL);
}

void vtm_dataset_set_string(vtm_dataset *ds, const char *name, const char *val)
{
	VTM_DATASET_SETX(VTM_V_STR, vtm_str_copy(val), true);
}

const void* vtm_dataset_get_blob(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET(blob, NULL);
}

void vtm_dataset_set_blob(vtm_dataset *ds, const char *name, const void *val)
{
	VTM_DATASET_SETX(VTM_V_BLOB, (void*) val, true);
}

void* vtm_dataset_get_pointer(vtm_dataset *ds, const char *name)
{
	VTM_DATASET_GET(ptr, NULL);
}

void vtm_dataset_set_pointer(vtm_dataset *ds, const char *name, void *val)
{
	VTM_DATASET_SET(VTM_V_PTR);
}
