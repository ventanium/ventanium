/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file smap.h
 *
 * @brief Generic hashmap implementation
 */

#ifndef VTM_CORE_SMAP_H_
#define VTM_CORE_SMAP_H_

#include <vtm/core/api.h>
#include <vtm/core/elem.h>
#include <vtm/core/hash.h>
#include <vtm/core/macros.h>
#include <vtm/core/types.h>

/**
 * Defines the struct that holds the map.
 *
 * The NODE_TYPE struct must meet following requirements:
 * - a member with type "struct vtm_variant" named "key"
 * - a member with type "NODE_TYPE*" named "next"
 *
 * @param NODE_TYPE name of the struct whose instances are stored as
 *        entries in the map
 * @param BUCKETS the number of hash buckets used by the map
 */
#define VTM_SMAP_STRUCT(NODE_TYPE, BUCKETS)                      \
	struct {                                                     \
		NODE_TYPE* buckets[BUCKETS];                             \
		vtm_hash_elem_fn fn_key_hash;                            \
		vtm_elem_cmp_fn fn_key_cmp;                              \
		void (*fn_free_node)(NODE_TYPE *node);                   \
	}

/**
 * Initializes the map.
 *
 * This function must be called, before any of the other
 * functions can be used.
 *
 * @param NAME the target map variable
 * @param KEY_TYPE enum vtm_elem_type
 * @param FREE_FN function pointer that is called when a entry
 *        is removed from the map
 */
#define VTM_SMAP_INIT(NAME, KEY_TYPE, FREE_FN)                   \
	do {                                                         \
		size_t i;                                                \
		for (i=0; i < VTM_ARRAY_LEN(NAME.buckets); i++) {        \
			NAME.buckets[i] = NULL;                              \
		}                                                        \
		NAME.fn_key_hash = vtm_hash_elem_get_fn(KEY_TYPE);       \
		NAME.fn_key_cmp = vtm_elem_get_cmp_fn(KEY_TYPE);         \
		NAME.fn_free_node = FREE_FN;                             \
	} while (0)

/**
 * Sets an alternative hash function for the key.
 *
 * @param NAME the target map variable
 * @param HASH_FN function pointer to hash function
 * @param CMP_FN function pointer to key comparison function
 */
#define VTM_SMAP_SET_KEY_FN(NAME, HASH_FN, CMP_FN)               \
	do {                                                         \
		NAME.fn_key_hash = HASH_FN;                              \
		NAME.fn_key_cmp = CMP_FN;                                \
	} while(0)

/**
 * INTERNAL: Calculates the bucket index for given key.
 */
#define VTM_SMAP_INDEX(NAME, KEY_PTR)                            \
	(NAME).fn_key_hash(&((KEY_PTR)->data)) % VTM_ARRAY_LEN(NAME.buckets)

/**
 * INTERNAL: Frees the given node.
 */
#define VTM_SMAP_FREE_NODE(NAME, NODE_PTR)                       \
	do {                                                         \
		if (NAME.fn_free_node)                                   \
			NAME.fn_free_node(NODE_PTR);                         \
		free(NODE_PTR);                                          \
	} while (0)

/**
 * Stores a new value in the map.
 *
 * Existing values stored under the same key are removed first.
 *
 * @param NAME the target map variable
 * @param NODE_TYPE the type of the value
 * @param NODE_PTR the value that should be stored
 */
#define VTM_SMAP_PUT(NAME, NODE_TYPE, NODE_PTR)                  \
	do {                                                         \
		size_t index;                                            \
		index = VTM_SMAP_INDEX(NAME, &(NODE_PTR->key));          \
		if (NAME.buckets[index] == NULL) {                       \
			NAME.buckets[index] = NODE_PTR;                      \
		}                                                        \
		else {                                                   \
			NODE_TYPE *cur, *prev;                               \
			bool added;                                          \
			added = false;                                       \
			cur = NAME.buckets[index];                           \
			prev = NULL;                                         \
			while (true) {                                       \
				if (NAME.fn_key_cmp(&(cur->key.data),            \
					 &(NODE_PTR->key.data))) {                   \
					if (prev)                                    \
						prev->next = NODE_PTR;                   \
					else                                         \
						NAME.buckets[index] = NODE_PTR;          \
					NODE_PTR->next = cur->next;                  \
					VTM_SMAP_FREE_NODE(NAME, cur);               \
					added = true;                                \
					break;                                       \
				}                                                \
				if (!cur->next)                                  \
					break;                                       \
				prev = cur;                                      \
				cur = cur->next;                                 \
			}                                                    \
			if (!added)                                          \
				cur->next = NODE_PTR;                            \
		}                                                        \
	} while (0)

/**
 * Retrieves a stored value from the map.
 *
 * @param NAME the target map variable
 * @param NODE_TYPE the type of the value
 * @param KEY_PTR pointer to the key
 * @param[out] OUT_PTR the retrieved value node
 */
#define VTM_SMAP_GET(NAME, NODE_TYPE, KEY_PTR, OUT_PTR)          \
	do {                                                         \
		size_t index;                                            \
		index = VTM_SMAP_INDEX(NAME, KEY_PTR);                   \
		if (NAME.buckets[index] != NULL) {                       \
			NODE_TYPE *cur;                                      \
			cur = NAME.buckets[index];                           \
			for(; cur != NULL; cur = cur->next) {                \
				if (NAME.fn_key_cmp(&(cur->key.data),            \
					 &((KEY_PTR)->data))) {                      \
					OUT_PTR = cur;                               \
					break;                                       \
				}                                                \
			}                                                    \
		}                                                        \
	} while (0)

/**
 * Removes a value from the map.
 *
 * @param NAME the target map variable
 * @param NODE_TYPE the type of the value
 * @param KEY_PTR pointer to the key
 * @param[out] OUT_REMOVED true if the value was found and removed
 */
#define VTM_SMAP_REMOVE(NAME, NODE_TYPE, KEY_PTR, OUT_REMOVED)   \
	do {                                                         \
		size_t index;                                            \
		OUT_REMOVED = false;                                     \
		index = VTM_SMAP_INDEX(NAME, KEY_PTR);                   \
		if (NAME.buckets[index] != NULL) {                       \
			NODE_TYPE *cur, *prev;                               \
			prev = NULL;                                         \
			cur = NAME.buckets[index];                           \
			while (cur) {                                        \
				if (NAME.fn_key_cmp(&(cur->key.data),            \
					 &((KEY_PTR)->data))) {                      \
					if (prev)                                    \
						prev->next = cur->next;                  \
					else                                         \
						NAME.buckets[index] = cur->next;         \
					VTM_SMAP_FREE_NODE(NAME, cur);               \
					OUT_REMOVED = true;                          \
					break;                                       \
				}                                                \
				prev = cur;                                      \
				cur = cur->next;                                 \
			}                                                    \
		}                                                        \
	} while (0)

/**
 * Iterates through all stored values and allows modifications
 * of the map.
 *
 * @param NAME the target map variable
 * @param BC bucket counter variable
 * @param[out] NODE_PTR holds current value
 * @param[out] NEXT_PTR hold next value
 */
#define VTM_SMAP_FOR_EACH_MOD(NAME, BC, NODE_PTR, NEXT_PTR)      \
	for (BC=0; BC < VTM_ARRAY_LEN(NAME.buckets); BC++)           \
		for (NODE_PTR = NAME.buckets[BC],                        \
			NEXT_PTR = NODE_PTR ? NODE_PTR->next : NULL;         \
			NODE_PTR != NULL; NODE_PTR = NEXT_PTR,               \
			NEXT_PTR = NEXT_PTR ? NEXT_PTR->next : NULL)

/**
 * Iterates through all stored values.
 *
 * @param NAME the target map variable
 * @param BC bucket counter variable
 * @param[out] NODE_PTR holds current value
 */
#define VTM_SMAP_FOR_EACH(NAME, BC, NODE_PTR)                    \
	for (BC=0; BC < VTM_ARRAY_LEN(NAME.buckets); BC++)           \
		for (NODE_PTR=NAME.buckets[BC]; NODE_PTR != NULL;        \
			NODE_PTR = NODE_PTR->next)

/**
 * Retrieves the number of stored values.
 *
 * @param NAME the target map variable
 * @param BC bucket counter variable
 * @param NODE_PTR pointer to value type
 * @param[out] COUNTER holds the result
 */
#define VTM_SMAP_GET_SIZE(NAME, BC, NODE_PTR, COUNTER)           \
	VTM_SMAP_FOR_EACH(NAME, BC, NODE_PTR)                        \
		COUNTER++

/**
 * Removes all values from the map.
 *
 * @param NAME the target map variable
 * @param NODE_TYPE the type of the values
 */
#define VTM_SMAP_CLEAR(NAME, NODE_TYPE)                          \
	do {                                                         \
		size_t bc;                                               \
		bc = 0;                                                  \
		NODE_TYPE *ptr;                                          \
		NODE_TYPE *next;                                         \
		VTM_SMAP_FOR_EACH_MOD(NAME, bc, ptr, next) {             \
			VTM_SMAP_FREE_NODE(NAME, ptr);                       \
		}                                                        \
		for (bc=0; bc < VTM_ARRAY_LEN(NAME.buckets); bc++)       \
			NAME.buckets[bc] = NULL;                             \
	} while (0)

#endif /* VTM_CORE_SMAP_H_ */
