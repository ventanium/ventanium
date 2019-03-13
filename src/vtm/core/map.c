/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "map.h"

#include <stdarg.h>
#include <vtm/core/error.h>
#include <vtm/core/hash.h>
#include <vtm/core/string.h>

#define VTM_MAP_KEY_NUM(UELEM, TYPE)                               \
	struct vtm_map_key result;                                     \
	result.var.data.elem_ ## UELEM = val;                          \
	result.var.type = TYPE;                                        \
	result.hash = vtm_hash_elem_## UELEM(&(result.var.data));      \
	return result;

struct vtm_map_node
{
	struct vtm_map_entry entry;
	struct vtm_map_node *next;
};

struct vtm_map
{
	enum vtm_elem_type keytype;
	enum vtm_elem_type valuetype;
	struct vtm_map_node **buckets;
	size_t bucket_count;
	vtm_elem_cmp_fn fn_key_cmp;
	vtm_map_free_func fn_free_val;
};

/* forward declaration */
static size_t vtm_map_calc_index(vtm_map *map, uint32_t hash);
static void vtm_map_parse_key(vtm_map *map, va_list *ap, struct vtm_map_key *key, bool copy);
static void vtm_map_parse_value(vtm_map *map, va_list *ap, union vtm_elem *dst);
static struct vtm_map_node** vtm_map_get_node_pos(vtm_map *map, struct vtm_map_key *key);
static struct vtm_map_node** vtm_map_get_node_pos_va(vtm_map *map, va_list *ap);
static struct vtm_map_node** vtm_map_find_node_pos(vtm_map *map, union vtm_elem *key, uint32_t hash);
static struct vtm_map_node** vtm_map_find_bucket_node_pos(vtm_map *map, struct vtm_map_node **head, union vtm_elem *key);
static int vtm_map_put_internal(vtm_map *map, struct vtm_map_key *key, union vtm_elem *value);
static union vtm_elem* vtm_map_get_internal(vtm_map *map, struct vtm_map_node **pos);
static void* vtm_map_get_pointer_internal(vtm_map *map, struct vtm_map_node **pos);
static bool vtm_map_remove_internal(vtm_map *map, struct vtm_map_node **pos);
static void vtm_map_node_free(vtm_map *map, struct vtm_map_node *node);

vtm_map* vtm_map_new(enum vtm_elem_type keytype, enum vtm_elem_type valuetype, size_t capacity)
{
	vtm_map *map;

	map = malloc(sizeof(vtm_map));
	if (!map) {
		vtm_err_oom();
		return NULL;
	}

	map->keytype = keytype;
	map->valuetype = valuetype;
	map->fn_key_cmp = vtm_elem_get_cmp_fn(keytype);
	map->fn_free_val = NULL;

	map->bucket_count = 1;
	while (map->bucket_count < capacity)
		map->bucket_count = map->bucket_count << 1;

	map->buckets = calloc(map->bucket_count, sizeof(struct vtm_map_node*));
	if (!map->buckets) {
		vtm_err_oom();
		free(map);
		return NULL;
	}

	return map;
}

void vtm_map_free(vtm_map *map)
{
	if (map == NULL)
		return;

	vtm_map_clear(map);
	free(map->buckets);
	free(map);
}

void vtm_map_set_free_func(vtm_map *map, vtm_map_free_func func)
{
	map->fn_free_val = func;
}

size_t vtm_map_size(vtm_map *map)
{
	size_t size;
	size_t i;
	struct vtm_map_node *node;

	size = 0;
	for (i=0; i < map->bucket_count; i++)
		for (node = map->buckets[i]; node != NULL; node = node->next)
			size++;

	return size;
}

void vtm_map_clear(vtm_map *map)
{
	size_t i;
	struct vtm_map_node *node;
	struct vtm_map_node *next;

	for (i=0; i < map->bucket_count; i++) {
		for (node = map->buckets[i], next = node ? node->next : NULL;
			 node != NULL; node = next, next = next ? next->next : NULL) {
			vtm_map_node_free(map, node);
		}
		map->buckets[i] = NULL;
	}
}

int vtm_map_put(vtm_map *map, struct vtm_map_key key, struct vtm_variant value)
{
	return vtm_map_put_internal(map, &key, &(value.data));
}

int vtm_map_put_va(vtm_map *map, ...)
{
	va_list ap;
	struct vtm_map_key key;
	union vtm_elem value;

	va_start(ap, map);
	vtm_map_parse_key(map, &ap, &key, true);
	vtm_map_parse_value(map, &ap, &value);
	va_end(ap);

	return vtm_map_put_internal(map, &key, &value);
}

static int vtm_map_put_internal(vtm_map *map, struct vtm_map_key *key, union vtm_elem *value)
{
	size_t index;
	struct vtm_map_node *node;
	struct vtm_map_node **pos;

	node = malloc(sizeof(*node));
	if (!node) {
		vtm_err_oom();
		return VTM_E_MALLOC;
	}

	node->entry.key = key->var.data;
	node->entry.value = *value;
	node->next = NULL;

	index = vtm_map_calc_index(map, key->hash);
	if (map->buckets[index] == NULL) {
		map->buckets[index] = node;
	}
	else {
		pos = vtm_map_find_bucket_node_pos(map, &(map->buckets[index]), &(node->entry.key));
		if (pos != NULL) {
			node->next = (*pos)->next;
			vtm_map_node_free(map, *pos);
			*pos = node;
		}
		else {
			node->next = map->buckets[index];
			map->buckets[index] = node;
		}
	}

	return VTM_OK;
}

union vtm_elem* vtm_map_get(vtm_map *map, struct vtm_map_key key)
{
	struct vtm_map_node **pos;

	pos = vtm_map_get_node_pos(map, &key);

	return vtm_map_get_internal(map, pos);
}

union vtm_elem* vtm_map_get_va(vtm_map *map, ...)
{
	va_list ap;
	struct vtm_map_node **pos;

	va_start(ap, map);
	pos = vtm_map_get_node_pos_va(map, &ap);
	va_end(ap);

	return vtm_map_get_internal(map, pos);
}

static union vtm_elem* vtm_map_get_internal(vtm_map *map, struct vtm_map_node **pos)
{
	if (pos)
		return &((*pos)->entry.value);

	return NULL;
}

void* vtm_map_get_pointer(vtm_map *map, struct vtm_map_key key)
{
	struct vtm_map_node **pos;

	pos = vtm_map_get_node_pos(map, &key);

	return vtm_map_get_pointer_internal(map, pos);
}

void* vtm_map_get_pointer_va(vtm_map *map, ...)
{
	va_list ap;
	struct vtm_map_node **pos;

	va_start(ap, map);
	pos = vtm_map_get_node_pos_va(map, &ap);
	va_end(ap);

	return vtm_map_get_pointer_internal(map, pos);
}

static void* vtm_map_get_pointer_internal(vtm_map *map, struct vtm_map_node **pos)
{
	if (pos)
		return (*pos)->entry.value.elem_pointer;

	return NULL;
}

bool vtm_map_remove(vtm_map *map, struct vtm_map_key key)
{
	struct vtm_map_node **pos;

	pos = vtm_map_get_node_pos(map, &key);

	return vtm_map_remove_internal(map, pos);
}

bool vtm_map_remove_va(vtm_map *map, ...)
{
	va_list ap;
	struct vtm_map_node **pos;

	va_start(ap, map);
	pos = vtm_map_get_node_pos_va(map, &ap);
	va_end(ap);

	return vtm_map_remove_internal(map, pos);
}

static bool vtm_map_remove_internal(vtm_map *map, struct vtm_map_node **pos)
{
	struct vtm_map_node *tmp;

	if (pos) {
		tmp = (*pos)->next;
		vtm_map_node_free(map, *pos);
		*pos = tmp;
		return true;
	}

	return false;
}

bool vtm_map_contains_key(vtm_map *map, struct vtm_map_key key)
{
	struct vtm_map_node **pos;

	pos = vtm_map_get_node_pos(map, &key);

	return pos != NULL;
}

bool vtm_map_contains_key_va(vtm_map *map, ...)
{
	va_list ap;
	struct vtm_map_node **pos;

	va_start(ap, map);
	pos = vtm_map_get_node_pos_va(map, &ap);
	va_end(ap);

	return pos != NULL;
}

vtm_list* vtm_map_entryset(vtm_map *map)
{
	vtm_list *result;
	size_t entries, i;
	struct vtm_map_node *node;

	entries = vtm_map_size(map);
	result = vtm_list_new(VTM_ELEM_POINTER, entries);
	if (!result)
		return NULL;

	for (i=0; i < map->bucket_count; i++)
		for (node = map->buckets[i]; node != NULL; node = node->next)
			vtm_list_add_va(result, &(node->entry));

	return result;
}

static size_t vtm_map_calc_index(vtm_map *map, uint32_t hash)
{
	return hash % map->bucket_count;
}

static void vtm_map_parse_key(vtm_map *map, va_list *ap, struct vtm_map_key *key, bool copy)
{
	vtm_elem_parse(map->keytype, &(key->var.data), ap);
	vtm_hash_elem_fn fn = vtm_hash_elem_get_fn(map->keytype);
	key->hash = fn(&(key->var.data));
	key->var.type = map->keytype;

	if (!copy)
		return;

	switch (map->keytype) {
		case VTM_ELEM_STRING:
			key->var.data.elem_pointer = vtm_str_copy(key->var.data.elem_pointer);
			break;

		default:
			break;
	}
}

static void vtm_map_parse_value(vtm_map *map, va_list *ap, union vtm_elem *dst)
{
	vtm_elem_parse(map->valuetype, dst, ap);
}

static struct vtm_map_node** vtm_map_get_node_pos(vtm_map *map, struct vtm_map_key *key)
{
	return vtm_map_find_node_pos(map, &(key->var.data), key->hash);
}

static struct vtm_map_node** vtm_map_get_node_pos_va(vtm_map *map, va_list *ap)
{
	struct vtm_map_key key;

	vtm_map_parse_key(map, ap, &key, false);

	return vtm_map_get_node_pos(map, &key);
}

static struct vtm_map_node** vtm_map_find_node_pos(vtm_map *map, union vtm_elem *key, uint32_t hash)
{
	size_t index;

	index = vtm_map_calc_index(map, hash);
	if (map->buckets[index] == NULL)
		return NULL;

	return vtm_map_find_bucket_node_pos(map, &(map->buckets[index]), key);
}

static struct vtm_map_node** vtm_map_find_bucket_node_pos(vtm_map *map, struct vtm_map_node **head, union vtm_elem *key)
{
	struct vtm_map_node **prev;

	for (prev = head; *prev != NULL; prev = &((*prev)->next)) {
		if (map->fn_key_cmp(&((*prev)->entry.key), key))
			return prev;
	}

	return NULL;
}

static void vtm_map_node_free(vtm_map *map, struct vtm_map_node *node)
{
	if (!node)
		return;

	switch (map->keytype) {
		case VTM_ELEM_STRING:
			free(node->entry.key.elem_pointer);
			break;

		default:
			break;
	}

	switch (map->valuetype) {
		case VTM_ELEM_STRING:
			free(node->entry.value.elem_pointer);
			break;

		case VTM_ELEM_POINTER:
			if (map->fn_free_val != NULL)
				map->fn_free_val(node->entry.value.elem_pointer);
			break;

		default:
			break;
	}

	free(node);
}

/* ########## KEY CONVERSION ########## */

struct vtm_map_key vtm_map_key_from_int8(int8_t val)
{
	VTM_MAP_KEY_NUM(int8, VTM_ELEM_INT8);
}

struct vtm_map_key vtm_map_key_from_uint8(uint8_t val)
{
	VTM_MAP_KEY_NUM(uint8, VTM_ELEM_UINT8);
}

struct vtm_map_key vtm_map_key_from_int16(int16_t val)
{
	VTM_MAP_KEY_NUM(int16, VTM_ELEM_INT16);
}

struct vtm_map_key vtm_map_key_from_uint16(uint16_t val)
{
	VTM_MAP_KEY_NUM(uint16, VTM_ELEM_UINT16);
}

struct vtm_map_key vtm_map_key_from_int32(int32_t val)
{
	VTM_MAP_KEY_NUM(int32, VTM_ELEM_INT32);
}

struct vtm_map_key vtm_map_key_from_uint32(uint32_t val)
{
	VTM_MAP_KEY_NUM(uint32, VTM_ELEM_UINT32);
}

struct vtm_map_key vtm_map_key_from_int64(int64_t val)
{
	VTM_MAP_KEY_NUM(int64, VTM_ELEM_INT64);
}

struct vtm_map_key vtm_map_key_from_uint64(uint64_t val)
{
	VTM_MAP_KEY_NUM(uint64, VTM_ELEM_UINT64);
}

struct vtm_map_key vtm_map_key_from_bool(bool val)
{
	VTM_MAP_KEY_NUM(bool, VTM_ELEM_BOOL);
}

struct vtm_map_key vtm_map_key_from_char(char val)
{
	VTM_MAP_KEY_NUM(char, VTM_ELEM_CHAR);
}

struct vtm_map_key vtm_map_key_from_schar(signed char val)
{
	VTM_MAP_KEY_NUM(schar, VTM_ELEM_SCHAR);
}

struct vtm_map_key vtm_map_key_from_uchar(unsigned char val)
{
	VTM_MAP_KEY_NUM(uchar, VTM_ELEM_UCHAR);
}

struct vtm_map_key vtm_map_key_from_short(short val)
{
	VTM_MAP_KEY_NUM(short, VTM_ELEM_SHORT);
}

struct vtm_map_key vtm_map_key_from_ushort(unsigned short val)
{
	VTM_MAP_KEY_NUM(ushort, VTM_ELEM_USHORT);
}

struct vtm_map_key vtm_map_key_from_int(int val)
{
	VTM_MAP_KEY_NUM(int, VTM_ELEM_INT);
}

struct vtm_map_key vtm_map_key_from_uint(unsigned int val)
{
	VTM_MAP_KEY_NUM(uint, VTM_ELEM_UINT);
}

struct vtm_map_key vtm_map_key_from_long(long val)
{
	VTM_MAP_KEY_NUM(long, VTM_ELEM_LONG);
}

struct vtm_map_key vtm_map_key_from_ulong(unsigned long val)
{
	VTM_MAP_KEY_NUM(ulong, VTM_ELEM_ULONG);
}

struct vtm_map_key vtm_map_key_from_float(float val)
{
	VTM_MAP_KEY_NUM(float, VTM_ELEM_FLOAT);
}

struct vtm_map_key vtm_map_key_from_double(double val)
{
	VTM_MAP_KEY_NUM(double, VTM_ELEM_DOUBLE);
}

struct vtm_map_key vtm_map_key_from_str(const char *val)
{
	struct vtm_map_key result;
	result.var.data.elem_pointer = (void*) val;
	result.var.type = VTM_ELEM_STRING;
	result.hash = vtm_hash_str(val);
	return result;
}

struct vtm_map_key vtm_map_key_from_ptr(void *val)
{
	struct vtm_map_key result;
	result.var.data.elem_pointer = val;
	result.var.type = VTM_ELEM_POINTER;
	result.hash = vtm_hash_ptr(val);
	return result;
}
