/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

/**
 * @file list.h
 *
 * @brief Elem based list
 */

#ifndef VTM_CORE_LIST_H_
#define VTM_CORE_LIST_H_

#include <vtm/core/api.h>
#include <vtm/core/elem.h>
#include <vtm/core/types.h>
#include <vtm/core/variant.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtm_list vtm_list;
typedef void (*vtm_list_free_func)(void *val);

/**
 * Creates a new list on the heap.
 *
 * @param type the type of items that should be stored in the list
 * @param init_size the initial size of the backing array
 * @return the created list
 * @return NULL if memory allocation failed or an invalid argument was given
 */
VTM_API vtm_list* vtm_list_new(enum vtm_elem_type type, size_t init_size);

/**
 * Releases the list and all allocated resources.
 *
 * After this call the list pointer is no longer valid.
 *
 * @param li the list that should be freed
 */
VTM_API void vtm_list_free(vtm_list *li);

/**
 * Removes all elements from the list.
 *
 * @param li the list that should be cleared
 */
VTM_API void vtm_list_clear(vtm_list *li);

/**
 * Get the current number of items stored in the list.
 *
 * @param li the list
 * @return the number of items in the list
 */
VTM_API size_t vtm_list_size(vtm_list *li);

/**
 * Adds a new item to the list.
 *
 * @param li the target list
 * @param var the value that should be stored in the list
 * @return VTM_OK if the value was sucessfully stored
 * @return VTM_E_INVALID arg if one of the given arguments is invalid
 * @return VTM_E_MALLOC if memory allocation failed
 */
VTM_API int vtm_list_add(vtm_list *li, struct vtm_variant var);

/**
 * Adds a new item to the list.
 *
 * This method expects the value as variadic argument after the list parameter.
 *
 * @param li the target list
 * @return VTM_OK if the value was sucessfully stored
 * @return VTM_E_INVALID arg if one of the given arguments is invalid
 * @return VTM_E_MALLOC if memory allocation failed
 */
VTM_API int vtm_list_add_va(vtm_list *li, ...);

/**
 * Removes the value at the given index.
 *
 * The items after the removed item shift one place to the right.
 *
 * @param li the target list
 * @param index the index of the item that should be removed
 * @return VTM_OK if the item was sucessfully removed
 * @return VTM_E_OUT_OF_RANGE if the given index is invalid
 */
VTM_API int vtm_list_remove(vtm_list *li, size_t index);

/**
 * Sets a callback that is called whenever a pointer value is removed
 * from the list.
 *
 * @param li the list where the callback should be set
 * @param fn function pointer to callback
 */
VTM_API void vtm_list_set_free_func(vtm_list *li, vtm_list_free_func fn);

VTM_API int8_t vtm_list_get_int8(vtm_list *li, size_t index);
VTM_API uint8_t vtm_list_get_uint8(vtm_list *li, size_t index);

VTM_API int16_t vtm_list_get_int16(vtm_list *li, size_t index);
VTM_API uint16_t vtm_list_get_uint16(vtm_list *li, size_t index);

VTM_API int32_t vtm_list_get_int32(vtm_list *li, size_t index);
VTM_API uint32_t vtm_list_get_uint32(vtm_list *li, size_t index);

VTM_API int64_t vtm_list_get_int64(vtm_list *li, size_t index);
VTM_API uint64_t vtm_list_get_uint64(vtm_list *li, size_t index);

VTM_API bool vtm_list_get_bool(vtm_list *li, size_t index);

VTM_API char vtm_list_get_char(vtm_list *li, size_t index);
VTM_API signed char vtm_list_get_schar(vtm_list *li, size_t index);
VTM_API unsigned char vtm_list_get_uchar(vtm_list *li, size_t index);

VTM_API short vtm_list_get_short(vtm_list *li, size_t index);
VTM_API unsigned short vtm_list_get_ushort(vtm_list *li, size_t index);

VTM_API int vtm_list_get_int(vtm_list *li, size_t index);
VTM_API unsigned int vtm_list_get_uint(vtm_list *li, size_t index);

VTM_API long vtm_list_get_long(vtm_list *li, size_t index);
VTM_API unsigned long vtm_list_get_ulong(vtm_list *li, size_t index);

VTM_API float vtm_list_get_float(vtm_list *li, size_t index);
VTM_API double vtm_list_get_double(vtm_list *li, size_t index);

VTM_API void* vtm_list_get_pointer(vtm_list *li, size_t index);

#ifdef __cplusplus
}
#endif

#endif /* VTM_CORE_LIST_H_ */
