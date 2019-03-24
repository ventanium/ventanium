/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "blob.h"

#include <stdlib.h>
#include <vtm/core/error.h>

struct vtm_blob
{
	uint32_t len;
	char mem[];
};

void* vtm_blob_new(size_t len)
{
	struct vtm_blob *blob;

	if (len > UINT32_MAX || len + sizeof(*blob) < len) {
		vtm_err_set(VTM_E_INVALID_ARG);
		return NULL;
	}

	blob = malloc(sizeof(*blob) + len);
	if (!blob) {
		vtm_err_oom();
		return NULL;
	}

	blob->len = (uint32_t) len;

	return blob->mem;
}

size_t vtm_blob_size(const void *blob)
{
	struct vtm_blob *header;

	header = (struct vtm_blob*) ((char*) blob - offsetof(struct vtm_blob, mem));
	return (size_t) header->len;
}

void vtm_blob_free(void *blob)
{
	free((char*) blob - offsetof(struct vtm_blob, mem));
}
