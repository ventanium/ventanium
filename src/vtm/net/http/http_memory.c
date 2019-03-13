/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "http_memory.h"

#include <string.h> /* memset() */
#include <vtm/core/error.h> /* result codes */
#include <vtm/core/map.h>
#include <vtm/util/mutex.h>
#include <vtm/net/http/http_memory_intl.h>

struct vtm_http_mem
{
	vtm_mutex *mtx;
	vtm_map *values;
	vtm_map *mutexes;
};

vtm_http_mem* vtm_http_mem_new(void)
{
	vtm_http_mem *mem;
	
	mem = malloc(sizeof(vtm_http_mem));
	if (!mem)
		return NULL;

	memset(mem, 0, sizeof(vtm_http_mem));
	mem->mtx = vtm_mutex_new();
	if (!mem->mtx)
		goto err;

	mem->values = vtm_map_new(VTM_ELEM_STRING, VTM_ELEM_POINTER, 16);
	if (!mem->values)
		goto err;
	
	mem->mutexes = vtm_map_new(VTM_ELEM_STRING, VTM_ELEM_POINTER, 16);
	if (!mem->mutexes) 
		goto err;
	
	return mem;

err:
	vtm_mutex_free(mem->mtx);
	vtm_map_free(mem->values);
	vtm_map_free(mem->mutexes);
	free(mem);
	return NULL;
}

void vtm_http_mem_free(vtm_http_mem *mem)
{
	vtm_list *entries;
	struct vtm_map_entry *entry;
	
	if (mem == NULL)
		return;
	
	/* values should have been freed external */	
	vtm_map_free(mem->values);
	
	/* free auto created mutexes */
	entries = vtm_map_entryset(mem->mutexes);
	if (entries) {
		size_t len;
		size_t i;
		
		len = vtm_list_size(entries);
		for (i=0; i < len; i++) {
			entry = vtm_list_get_pointer(entries, i);
			vtm_mutex_free(entry->value.elem_pointer);
		}
	}

	vtm_map_free(mem->mutexes);
	free(mem);
}

void vtm_http_mem_put(vtm_http_mem *mem, const char *key, void *val)
{
	vtm_mutex_lock(mem->mtx);
	vtm_map_put_va(mem->values, key, val);
	vtm_mutex_unlock(mem->mtx);
}

void* vtm_http_mem_get(vtm_http_mem *mem, const char *key)
{
	void *result;
	
	vtm_mutex_lock(mem->mtx);
	result = vtm_map_get_pointer_va(mem->values, key);
	vtm_mutex_unlock(mem->mtx);
	
	return result;
}

void vtm_http_mem_lock(vtm_http_mem *mem, const char *key)
{
	vtm_mutex *mtx;
	
	vtm_mutex_lock(mem->mtx);
	mtx = vtm_map_get_pointer_va(mem->mutexes, key);
	if (!mtx) {
		mtx = vtm_mutex_new();
		vtm_map_put_va(mem->mutexes, key);
	}
	vtm_mutex_lock(mtx);
	vtm_mutex_unlock(mem->mtx);
}

void vtm_http_mem_unlock(vtm_http_mem *mem, const char *key)
{
	vtm_mutex *mtx;
	
	vtm_mutex_lock(mem->mtx);
	mtx = vtm_map_get_pointer_va(mem->mutexes, key);
	VTM_ASSERT(mtx);
	vtm_mutex_unlock(mtx);
	vtm_mutex_unlock(mem->mtx);
}
