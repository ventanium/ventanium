/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file core/hash.h
 *
 * @brief Hash functions
 */

#ifndef VTM_CORE_HASH_H_
#define VTM_CORE_HASH_H_

#include <vtm/core/api.h>
#include <vtm/core/types.h>
#include <vtm/core/elem.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Calculates hash for given input string.
 *
 * @param in NUL-terminated input string
 * @return calculated hash value
 */
VTM_API uint32_t vtm_hash_str(const char *in);

/**
 * Calculates case-independent hash for given input string.
 *
 * @param in NUL-terminated input string
 * @return calculated hash value
 */
VTM_API uint32_t vtm_hash_strcase(const char *in);

/**
 * Calculates hash for given pointer.
 *
 * @param in a pointer
 * @return calculated hash value
 */
VTM_API uint32_t vtm_hash_ptr(const void *in);

/**
 * Calculates hash over given memory region.
 *
 * @param in pointer to memory region
 * @param len length in bytes of memory region
 * @return calculated hash value
 */
VTM_API uint32_t vtm_hash_mem(const void *in, size_t len);

/**
 * Calculates hash for given signed integer.
 *
 * @param in 64bit signed integer
 * @return calculated hash value
 */
VTM_API uint32_t vtm_hash_num(int64_t in);

/**
 * Calculates hash for given unsigned integer.
 *
 * @param in 64bit unsigned integer
 * @return calculated hash value
 */
VTM_API uint32_t vtm_hash_unum(uint64_t in);

/**
 * Calculates the hash for given element.
 *
 * @param el the element whose hash value should be calculated
 * @return the hash value
 */
typedef uint32_t (*vtm_hash_elem_fn)(union vtm_elem *el);

/**
 * Gets a function pointer to the appropriate element hash function.
 *
 * @param type the element type
 * @return function pointer to element hash function
 */
VTM_API vtm_hash_elem_fn vtm_hash_elem_get_fn(enum vtm_elem_type type);

/* element hash functions */
VTM_API uint32_t vtm_hash_elem_int8(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_uint8(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_int16(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_uint16(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_int32(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_uint32(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_int64(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_uint64(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_bool(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_char(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_schar(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_uchar(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_short(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_ushort(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_int(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_uint(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_long(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_ulong(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_float(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_double(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_str(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_strcase(union vtm_elem *el);
VTM_API uint32_t vtm_hash_elem_ptr(union vtm_elem *el);

#ifdef __cplusplus
}
#endif

#endif /* VTM_CORE_HASH_H_ */
