/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <stdio.h>
#include <vtm/core/error.h>
#include <vtm/core/list.h>

int main(void)
{
	int i;
	size_t k, count;
	vtm_list *li;

	/*
	 * Create a new list which should hold integer values.
	 * Initial capacity should be 8.
	 */
	li = vtm_list_new(VTM_ELEM_INT, 8);
	if (!li) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/*
	 * Add items with varargs. There is no type checking, so you must
	 * ensure that the argument has the correct type
	 */
	for (i=0; i < 64; i++) {
		vtm_list_add_va(li, i);
	}

	/* Add items the safe way */
	for (i=0; i < 64; i++) {
		vtm_list_add(li, VTM_V_INT(i));
	}

	/* The list has grown from the inital 8 elements to 128 */
	count = vtm_list_size(li);
	printf("Size of list: %d\n", (int) count);

	/* display all values */
	for (k=0; k < count; k++) {
		if (k > 0)
			printf(", ");
		printf("%d", vtm_list_get_int(li, k));
	}
	printf("\n");

	/* free resources */
	vtm_list_free(li);

	return 0;
}
