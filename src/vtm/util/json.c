/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "json.h"

#include <vtm/core/error.h>

int vtm_json_encode_ds(vtm_dataset *ds, struct vtm_buf *buf)
{
	vtm_list *entries;
	struct vtm_dataset_entry *entry;
	size_t i, count;
	bool quotes;

	entries = vtm_dataset_entryset(ds);
	if (!entries)
		return vtm_err_get_code();

	vtm_buf_putc(buf, '{');

	count = vtm_list_size(entries);
	for (i=0; i < count; i++) {
		entry = vtm_list_get_pointer(entries, i);
		quotes = entry->var->type == VTM_ELEM_STRING;

		if (i > 0)
			vtm_buf_putc(buf, ',');

		vtm_buf_putc(buf, '"');
		vtm_buf_puts(buf, entry->name);
		vtm_buf_putc(buf, '"');
		vtm_buf_putc(buf, ':');

		if (quotes)
			vtm_buf_putc(buf, '"');

		vtm_buf_puts(buf, vtm_variant_as_str(entry->var));

		if (quotes)
			vtm_buf_putc(buf, '"');
	}

	vtm_buf_putc(buf, '}');

	vtm_list_free(entries);

	return buf->err;
}
