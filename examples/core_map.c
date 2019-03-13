/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <stdlib.h>
#include <time.h>
#include <vtm/core/error.h>
#include <vtm/core/string.h>
#include <vtm/core/map.h>

const char* ANIMALS[] = {
	"Cat", "Dog", "Lion"
};

int main(void)
{
	int i, r;
	vtm_map *map;
	vtm_list *entries;
	struct vtm_map_entry *entry;
	union vtm_elem *val;
	size_t j, count;

	/*
	 * Create a new map which uses integers as keys and stores string values.
	 * The initial capacity should be 8.
	 */
	map = vtm_map_new(VTM_ELEM_INT, VTM_ELEM_STRING, 8);
	if (!map) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* Initialize random number generator */
	srand(time(NULL));

	/*
	 * Add entries to the map with varargs. You must ensure that
	 * the method is called with the correct type for key and value.
	 * Also note that the random number will not be evenly distributed
	 * because of modulus division bias.
	 */
	for (i=0; i < 16; i++) {
		r = rand() % 3;
		vtm_map_put_va(map, i, ANIMALS[r]);
	}

	/*
	 * Add entries to the map with typesafe arguments
	 */
	for (i=16; i < 32; i++) {
		r = rand() % 3;
		vtm_map_put(map, VTM_MK_INT(i), VTM_MV_STR(vtm_str_copy(ANIMALS[r])));
	}

	/* display entries with varargs */
	for (i=0; i < 16; i++) {
		val = vtm_map_get_va(map, i);
		if (val)
			printf("For value %d is a %s stored\n", i,
			(char*) val->elem_pointer);
	}

	/* display entries with safe args */
	for (i=16; i < 32; i++) {
		val = vtm_map_get(map, VTM_MK_INT(i));
		if (val)
			printf("For value %d is a %s stored\n", i,
			(char*) val->elem_pointer);
	}

	/* Iterate trough entries */
	printf("Iterating through entries:\n");
	entries = vtm_map_entryset(map);
	if (entries) {
		count = vtm_list_size(entries);
		for (j=0; j < count; j++) {
			entry = vtm_list_get_pointer(entries, j);
			printf("For value %d is a %s stored\n",
				entry->key.elem_int,
				(char*) entry->value.elem_pointer);
		}

		/* free entries list */
		vtm_list_free(entries);
	}

	/* free map */
	vtm_map_free(map);

	return 0;
}
