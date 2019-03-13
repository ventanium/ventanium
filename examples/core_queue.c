/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <stdio.h>
#include <vtm/core/error.h>
#include <vtm/core/squeue.h>

struct list_node
{
	int value;
	struct list_node *next;
};

int main(void)
{
	int i, count;
	struct list_node *node, *prev, *next;

	/* declare queue variable which contains the necessary pointers */
	VTM_SQUEUE_STRUCT(struct list_node) list;

	/* initialize queue */
	VTM_SQUEUE_INIT(list);

	/* add items to queue */
	for (i=0; i < 10; i++) {

		/* prepare new node */
		node = malloc(sizeof(*node));
		if (!node)
			goto end;

		node->value = i;
		VTM_SQUEUE_ADD(list, node);
	}

	/* remove first two numbers */
	count = 0;
	while (!VTM_SQUEUE_IS_EMPTY(list) && count++ < 2) {
		VTM_SQUEUE_POLL(list, node);
		printf("removed %d from head of list\n", node->value);
	}

	/* remove odd numbers */
	VTM_SQUEUE_FOR_EACH_MOD(list, node, prev, next) {
		if (node->value % 2 != 0)
			VTM_SQUEUE_REMOVE(list, node, prev, free);
	}

	/* display contents */
	count = 0;
	VTM_SQUEUE_FOR_EACH(list, node) {
		if (count++ > 0)
			printf(", ");
		printf("%d", node->value);
	}
	printf("\n");

end:
	/* remove all nodes from the queue */
	VTM_SQUEUE_CLEAR(list, struct list_node, free);

	return 0;
}
