/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file map.h
 *
 * @brief Elem based hashmap
 */

#ifndef VTM_CORE_MAP_H_
#define VTM_CORE_MAP_H_

#include <vtm/core/api.h>
#include <vtm/core/elem.h>
#include <vtm/core/list.h>
#include <vtm/core/types.h>
#include <vtm/core/variant.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtm_map vtm_map;
typedef void (*vtm_map_free_func)(void *val);

struct vtm_map_entry
{
	union vtm_elem key;
	union vtm_elem value;
};

struct vtm_map_key
{
	struct vtm_variant var;   /**< key value */
	uint32_t hash;            /**< hash for key value */
};

/**
 * Creates a new map on the heap.
 *
 * @param keytype the type of the keys
 * @param valuetype the type of the values that are stored
 * @param capacity the estimated maximum number of entries that should be stored
 *        in the map
 */
VTM_API vtm_map* vtm_map_new(enum vtm_elem_type keytype, enum vtm_elem_type valuetype, size_t capacity);

/**
 * Releases the map and all stored values.
 *
 * After this call the map pointer is no longer valid.
 *
 * @param map the map that should be released
 */
VTM_API void vtm_map_free(vtm_map *map);

/**
 * Sets a callback function that is called when a pointer value is freed.
 *
 * @param map the map where the callback should be installed
 * @param func function pointer to callback
 */
VTM_API void vtm_map_set_free_func(vtm_map *map, vtm_map_free_func func);

/**
 * Retrieves the number of stored entries.
 *
 * @param map the target map
 * @return the number of entries contained in the map
 */
VTM_API size_t vtm_map_size(vtm_map *map);

/**
 * Removes all entries from the map.
 *
 * @param map the map that should be cleared
 */
VTM_API void vtm_map_clear(vtm_map *map);

/**
 * Retrieves a list with all entries contained in the map.
 *
 * @param map the target map
 * @return a list containing pointers to struct vtm_map_entry
 * @return NULL if memory allocation for the list failed
 */
VTM_API vtm_list* vtm_map_entryset(vtm_map *map);

/**
 * Store a new entry in the map.
 *
 * @param map the target map
 * @param key the key
 * @param val the value that should be stored
 * @return VTM_OK if the entry was sucessfully stored
 * @return VTM_E_MALLOC if memory allocation for entry failed
 */
VTM_API int vtm_map_put(vtm_map *map, struct vtm_map_key key, struct vtm_variant val);

/**
 * Store a new entry in the map.
 *
 * This method expects two variadic arguments after the map parameter:
 * - one for the key
 * - one for the value
 *
 * @param map the target map
 * @return VTM_OK if the entry was sucessfully stored
 * @return VTM_E_MALLOC if memory allocation for entry failed
 */
VTM_API int vtm_map_put_va(vtm_map *map, ...);

/**
 * Retrieve a stored value from the map.
 *
 * @param map the target map
 * @param key the key
 * @return pointer to stored value union
 * @return NULL if there is no value stored for the given key
 */
VTM_API union vtm_elem* vtm_map_get(vtm_map *map, struct vtm_map_key key);

/**
 * Retrieve a stored value from the map.
 *
 * This method expects the key as variadic argument after the map parameter.
 *
 * @param map the target map
 * @return pointer to stored value union
 * @return NULL if there is no value stored for the given key
 */
VTM_API union vtm_elem* vtm_map_get_va(vtm_map *map, ...);

/**
 * Convenience method for retrieving stored pointer value.
 *
 * @param map the target map
 * @param key the key
 * @return stored pointer
 * @return NULL if there is no value stored for the given key
 */
VTM_API void* vtm_map_get_pointer(vtm_map *map, struct vtm_map_key key);

/**
 * Convenience method for retrieving stored pointer value.
 *
 * This method expects the key as variadic argument after the map parameter.
 *
 * @param map the target map
 * @return stored pointer
 * @return NULL if there is no value stored for the given key
 */
VTM_API void* vtm_map_get_pointer_va(vtm_map *map, ...);

/**
 * Removes an entry from the map.
 *
 * @param map the target map
 * @param key the key
 * @return true if the entry was found and removed
 * @return false if there was no entry for the given key
 */
VTM_API bool vtm_map_remove(vtm_map *map, struct vtm_map_key key);

/**
 * Removes an entry from the map.
 *
 * This method expects the key as variadic argument after the map parameter.
 *
 * @param map the target map
 * @return true if the entry was found and removed
 * @return false if there was no entry for the given key
 */
VTM_API bool vtm_map_remove_va(vtm_map *map, ...);

/**
 * Checks if the map contains a given key.
 *
 * @param map the target map
 * @param key the key that should be checked
 * @return true if the map contains an entry for the key
 * @return false if the map does not contain the key
 */
VTM_API bool vtm_map_contains_key(vtm_map *map, struct vtm_map_key key);

/**
 * Checks if the map contains a given key.
 *
 * This method expects the key as variadic argument after the map parameter.
 *
 * @param map the target map
 * @return true if the map contains an entry for the key
 * @return false if the map does not contain the key
 */
VTM_API bool vtm_map_contains_key_va(vtm_map *map, ...);

/* key conversion functions */
VTM_API struct vtm_map_key vtm_map_key_from_int8(int8_t val);
VTM_API struct vtm_map_key vtm_map_key_from_uint8(uint8_t val);
VTM_API struct vtm_map_key vtm_map_key_from_int16(int16_t val);
VTM_API struct vtm_map_key vtm_map_key_from_uint16(uint16_t val);
VTM_API struct vtm_map_key vtm_map_key_from_int32(int32_t val);
VTM_API struct vtm_map_key vtm_map_key_from_uint32(uint32_t val);
VTM_API struct vtm_map_key vtm_map_key_from_int64(int64_t val);
VTM_API struct vtm_map_key vtm_map_key_from_uint64(uint64_t val);
VTM_API struct vtm_map_key vtm_map_key_from_bool(bool val);
VTM_API struct vtm_map_key vtm_map_key_from_char(char val);
VTM_API struct vtm_map_key vtm_map_key_from_schar(signed char val);
VTM_API struct vtm_map_key vtm_map_key_from_uchar(unsigned char val);
VTM_API struct vtm_map_key vtm_map_key_from_short(short val);
VTM_API struct vtm_map_key vtm_map_key_from_ushort(unsigned short);
VTM_API struct vtm_map_key vtm_map_key_from_int(int val);
VTM_API struct vtm_map_key vtm_map_key_from_uint(unsigned int val);
VTM_API struct vtm_map_key vtm_map_key_from_long(long val);
VTM_API struct vtm_map_key vtm_map_key_from_ulong(unsigned long val);
VTM_API struct vtm_map_key vtm_map_key_from_float(float val);
VTM_API struct vtm_map_key vtm_map_key_from_double(double val);
VTM_API struct vtm_map_key vtm_map_key_from_str(const char *val);
VTM_API struct vtm_map_key vtm_map_key_from_blob(const void *val);
VTM_API struct vtm_map_key vtm_map_key_from_ptr(void *val);

#define VTM_MK_INT8(VAL)    vtm_map_key_from_int8(VAL)
#define VTM_MK_UINT8(VAL)   vtm_map_key_from_uint8(VAL)
#define VTM_MK_INT16(VAL)   vtm_map_key_from_int16(VAL)
#define VTM_MK_UINT16(VAL)  vtm_map_key_from_uint16(VAL)
#define VTM_MK_INT32(VAL)   vtm_map_key_from_int32(VAL)
#define VTM_MK_UINT32(VAL)  vtm_map_key_from_uint32(VAL)
#define VTM_MK_INT64(VAL)   vtm_map_key_from_int64(VAL)
#define VTM_MK_UINT64(VAL)  vtm_map_key_from_uint64(VAL)
#define VTM_MK_BOOL(VAL)    vtm_map_key_from_bool(VAL)
#define VTM_MK_CHAR(VAL)    vtm_map_key_from_char(VAL)
#define VTM_MK_SCHAR(VAL)   vtm_map_key_from_schar(VAL)
#define VTM_MK_UCHAR(VAL)   vtm_map_key_from_uchar(VAL)
#define VTM_MK_SHORT(VAL)   vtm_map_key_from_short(VAL)
#define VTM_MK_USHORT(VAL)  vtm_map_key_from_ushort(VAL)
#define VTM_MK_INT(VAL)     vtm_map_key_from_int(VAL)
#define VTM_MK_UINT(VAL)    vtm_map_key_from_uint(VAL)
#define VTM_MK_LONG(VAL)    vtm_map_key_from_long(VAL)
#define VTM_MK_ULONG(VAL)   vtm_map_key_from_ulong(VAL)
#define VTM_MK_FLOAT(VAL)   vtm_map_key_from_floatVAL)
#define VTM_MK_DOUBLE(VAL)  vtm_map_key_from_double(VAL)
#define VTM_MK_STR(VAL)     vtm_map_key_from_str(VAL)
#define VTM_MK_BLOB(VAL)    vtm_map_key_from_blob(VAL)
#define VTM_MK_PTR(VAL)     vtm_map_key_from_ptr(VAL)

/* value conversion functions */
#define VTM_MV_NULL()       VTM_V_NULL()
#define VTM_MV_INT8(VAL)    VTM_V_INT8(VAL)
#define VTM_MV_UINT8(VAL)   VTM_V_UINT8(VAL)
#define VTM_MV_INT16(VAL)   VTM_V_INT16(VAL)
#define VTM_MV_UINT16(VAL)  VTM_V_UINT16(VAL)
#define VTM_MV_INT32(VAL)   VTM_V_INT32(VAL)
#define VTM_MV_UINT32(VAL)  VTM_V_UINT32(VAL)
#define VTM_MV_INT64(VAL)   VTM_V_INT64(VAL)
#define VTM_MV_UINT64(VAL)  VTM_V_UINT64(VAL)
#define VTM_MV_BOOL(VAL)    VTM_V_BOOL(VAL)
#define VTM_MV_CHAR(VAL)    VTM_V_CHAR(VAL)
#define VTM_MV_SCHAR(VAL)   VTM_V_SCHAR(VAL)
#define VTM_MV_UCHAR(VAL)   VTM_V_UCHAR(VAL)
#define VTM_MV_SHORT(VAL)   VTM_V_SHORT(VAL)
#define VTM_MV_USHORT(VAL)  VTM_V_USHORT(VAL)
#define VTM_MV_INT(VAL)     VTM_V_INT(VAL)
#define VTM_MV_UINT(VAL)    VTM_V_UINT(VAL)
#define VTM_MV_LONG(VAL)    VTM_V_LONG(VAL)
#define VTM_MV_ULONG(VAL)   VTM_V_ULONG(VAL)
#define VTM_MV_FLOAT(VAL)   VTM_V_FLOAT(VAL)
#define VTM_MV_DOUBLE(VAL)  VTM_V_DOUBLE(VAL)
#define VTM_MV_STR(VAL)     VTM_V_STR(VAL)
#define VTM_MV_BLOB(VAL)    VTM_V_BLOB(VAL)
#define VTM_MV_PTR(VAL)     VTM_V_PTR(VAL)

#ifdef __cplusplus
}
#endif

#endif /* VTM_CORE_MAP_H_ */
