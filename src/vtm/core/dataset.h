/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file dataset.h
 *
 * @brief Named value storage
 */

#ifndef VTM_CORE_DATASET_H_
#define VTM_CORE_DATASET_H_

#include <vtm/core/api.h>
#include <vtm/core/list.h>
#include <vtm/core/smap.h>
#include <vtm/core/types.h>
#include <vtm/core/variant.h>

#ifdef __cplusplus
extern "C" {
#endif

/** No special behaviour */
#define VTM_DS_HINT_DEFAULT                   0

/**
 * Field names are treated as static and
 * are not copied
 */
#define VTM_DS_HINT_STATIC_NAMES              1

/**
 * Field names are treated case insensitive
 */
#define VTM_DS_HINT_IGNORE_CASE               2

struct vtm_dataset
{
	/* internal */
	unsigned int hints;
	VTM_SMAP_STRUCT(struct vtm_dataset_node, 8) vars;
};

struct vtm_dataset_entry
{
	const char *name;         /**< field name */
	struct vtm_variant *var;  /**< field value */
};

typedef struct vtm_dataset vtm_dataset;

/**
 * Initializes a new dataset.
 *
 * @param[out] ds the dataset that should be initialized
 * @param hints one or a combination of hints
 */
VTM_API void vtm_dataset_init(vtm_dataset *ds, unsigned int hints);

/**
 * Releases the dataset.
 *
 * All stored values are also released.
 *
 * @param ds the dataset that should be released
 */
VTM_API void vtm_dataset_release(vtm_dataset *ds);

/**
 * Creates a new dataset on the heap with default hints.
 *
 * @return the created dataset
 * @return NULL if memory allocation failed
 */
VTM_API vtm_dataset* vtm_dataset_new(void);

/**
 * Creates a new dataset on the heap with given hints.
 *
 * @return the created dataset
 * @return NULL if memory allocation failed
 */
VTM_API vtm_dataset* vtm_dataset_newh(unsigned int hints);

/**
 * Releases the dataset and frees the pointer.
 *
 * After this call the dataset pointer is no longer valid.
 *
 * @param ds the dataset that should be freed
 */
VTM_API void vtm_dataset_free(vtm_dataset *ds);

/**
 * Removes all entries from the dataset.
 *
 * @param ds the dataset that should be cleared
 */
VTM_API void vtm_dataset_clear(vtm_dataset *ds);

/**
 * Retrieves a list containing all dataset entries.
 *
 * @param ds the dataset whose entry list should be retrieved
 */
VTM_API vtm_list* vtm_dataset_entryset(vtm_dataset *ds);

/**
 * Checks if the dataset has an entry with given name.
 *
 * @param ds the datset that should be checked
 * @param name the fieldname
 * @return true if the dataset has an entry with the given name
 * @return false otherwise
 */
VTM_API bool vtm_dataset_contains(vtm_dataset *ds, const char *name);

/**
 * Removes the entry with given name from the dataset.
 *
 * @param ds the dataset where the entry should be removed
 * @param name the fieldname
 * @return VTM_OK if the entry was successfully removed
 * @return VTM_E_NOT_FOUND if the dataset has no entry with given name
 */
VTM_API int vtm_dataset_remove(vtm_dataset *ds, const char *name);

VTM_API struct vtm_variant* vtm_dataset_get_variant(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_variant(vtm_dataset *ds, const char *name, struct vtm_variant *var);

VTM_API int8_t vtm_dataset_get_int8(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_int8(vtm_dataset *ds, const char *name, int8_t val);

VTM_API uint8_t vtm_dataset_get_uint8(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_uint8(vtm_dataset *ds, const char *name, uint8_t val);

VTM_API int16_t vtm_dataset_get_int16(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_int16(vtm_dataset *ds, const char *name, int16_t val);

VTM_API uint16_t vtm_dataset_get_uint16(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_uint16(vtm_dataset *ds, const char *name, uint16_t val);

VTM_API int32_t vtm_dataset_get_int32(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_int32(vtm_dataset *ds, const char *name, int32_t val);

VTM_API uint32_t vtm_dataset_get_uint32(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_uint32(vtm_dataset *ds, const char *name, uint32_t val);

VTM_API int64_t vtm_dataset_get_int64(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_int64(vtm_dataset *ds, const char *name, int64_t val);

VTM_API uint64_t vtm_dataset_get_uint64(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_uint64(vtm_dataset *ds, const char *name, uint64_t val);

VTM_API bool vtm_dataset_get_bool(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_bool(vtm_dataset *ds, const char *name, bool val);

VTM_API char vtm_dataset_get_char(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_char(vtm_dataset *ds, const char *name, char val);

VTM_API signed char vtm_dataset_get_schar(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_schar(vtm_dataset *ds, const char *name, signed char val);

VTM_API unsigned char vtm_dataset_get_uchar(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_uchar(vtm_dataset *ds, const char *name, unsigned char val);

VTM_API short vtm_dataset_get_short(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_short(vtm_dataset *ds, const char *name, short val);

VTM_API unsigned short vtm_dataset_get_ushort(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_ushort(vtm_dataset *ds, const char *name, unsigned short val);

VTM_API int vtm_dataset_get_int(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_int(vtm_dataset *ds, const char *name, int val);

VTM_API unsigned int vtm_dataset_get_uint(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_uint(vtm_dataset *ds, const char *name, unsigned int val);

VTM_API long vtm_dataset_get_long(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_long(vtm_dataset *ds, const char *name, long val);

VTM_API unsigned long vtm_dataset_get_ulong(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_ulong(vtm_dataset *ds, const char *name, unsigned long val);

VTM_API float vtm_dataset_get_float(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_float(vtm_dataset *ds, const char *name, float val);

VTM_API double vtm_dataset_get_double(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_double(vtm_dataset *ds, const char *name, double val);

VTM_API const char* vtm_dataset_get_string(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_string(vtm_dataset *ds, const char *name, const char *val);

VTM_API void* vtm_dataset_get_pointer(vtm_dataset *ds, const char *name);
VTM_API void vtm_dataset_set_pointer(vtm_dataset *ds, const char *name, void *val);

#ifdef __cplusplus
}
#endif

#endif /* VTM_CORE_DATASET_H_ */
