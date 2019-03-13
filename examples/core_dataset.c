/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/core/error.h>
#include <vtm/core/dataset.h>

int main(void)
{
	vtm_dataset *ds;
	vtm_list *entries;
	struct vtm_dataset_entry *entry;
	size_t i, count;

	/* create new empty dataset */
	ds = vtm_dataset_new();
	if (!ds) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* put some values in to the dataset */
	vtm_dataset_set_string(ds, "PATH", "/tmp/");
	vtm_dataset_set_int(ds, "PORT", 80);
	vtm_dataset_set_double(ds, "FACTOR", 1.23456);
	vtm_dataset_set_float(ds, "DISTANCE", 3.14f);

	/* check for some values */
	if (vtm_dataset_contains(ds, "FACTOR")) {
		printf("FACTOR as string is %s\n",
			vtm_dataset_get_string(ds, "FACTOR"));
	}

	/* remove a value */
	vtm_dataset_remove(ds, "FACTOR");

	/* iterate trough contents */
	entries = vtm_dataset_entryset(ds);
	if (entries) {
		count = vtm_list_size(entries);
		for (i=0; i < count; i++) {
			entry = vtm_list_get_pointer(entries, i);
			printf("Key %s, value as string %s\n",
				entry->name, vtm_variant_as_str(entry->var));
		}

		/* free entries list */
		vtm_list_free(entries);
	}

	/* free resources */
	vtm_dataset_free(ds);

	return 0;
}
