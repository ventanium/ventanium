/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file squeue.h
 *
 * @brief Generic queue implementation
 */

#ifndef VTM_CORE_SQUEUE_H_
#define VTM_CORE_SQUEUE_H_

#include <vtm/core/api.h>
#include <vtm/core/macros.h>

/**
 * Defines the struct that holds the queue.
 *
 * The struct definition that represents the items in the queue must have
 * a member called "next" which must be a pointer to a struct instance.
 *
 * @param NODE_TYPE name of the struct whose instances are stored as items
 *        in the queue
 */
#define VTM_SQUEUE_STRUCT(NODE_TYPE)                             \
	struct {                                                     \
		NODE_TYPE *head;                                         \
		NODE_TYPE *tail;                                         \
	}

/**
 * Initializes the queue.
 *
 * This function must be called, before any of the other
 * functions can be used.
 *
 * @param NAME the target queue variable
 */
#define VTM_SQUEUE_INIT(NAME)                                    \
	do {                                                         \
		NAME.head = NULL;                                        \
		NAME.tail = NULL;                                        \
	} while (0)

/**
 * Adds an item to the queue.
 *
 * @param NAME the target queue variable
 * @param NODE_PTR pointer to struct instance that is added
 *        to the queue
 */
#define VTM_SQUEUE_ADD(NAME, NODE_PTR)                           \
	do {                                                         \
		NODE_PTR->next = NULL;                                   \
		if (NAME.tail)                                           \
			NAME.tail->next = NODE_PTR;                          \
		NAME.tail = NODE_PTR;                                    \
		if (!NAME.head)                                          \
			NAME.head = NODE_PTR;                                \
	} while (0)

/**
 * Retrieves and removes the head element of the queue.
 *
 * @param NAME the target queue variable
 * @param[out] NODE_PTR stores the retrieved element
 */
#define VTM_SQUEUE_POLL(NAME, NODE_PTR)                          \
	do {                                                         \
		NODE_PTR = NAME.head;                                    \
		if (NAME.head) {                                         \
			NAME.head = NAME.head->next;                         \
			if (NAME.tail == NODE_PTR)                           \
				NAME.tail = NULL;                                \
		}                                                        \
	} while (0)

/**
 * Removes the given item from the queue.
 *
 * @param NAME the target queue variable
 * @param NODE_PTR the item that should be removed
 * @param PREV_PTR the predecessor of the item that should be removed
 * @param FUNC function pointer that is called with the removed element
 */
#define VTM_SQUEUE_REMOVE(NAME, NODE_PTR, PREV_PTR, FUNC)        \
	do {                                                         \
		if (PREV_PTR)                                            \
			PREV_PTR->next = NODE_PTR->next;                     \
		else                                                     \
			NAME.head = NODE_PTR->next;                          \
		if (NODE_PTR == NAME.tail)                               \
			NAME.tail = PREV_PTR;                                \
		FUNC(NODE_PTR);                                          \
	} while (0)

/**
 * Checks wether the queue is empty.
 *
 * @param NAME the target queue variable
 */
#define VTM_SQUEUE_IS_EMPTY(NAME)                                \
	(NAME.head == NULL)

/**
 * Iterates through the queue.
 *
 * @param NAME the target queue variable
 * @param[out] NODE_PTR will point to current item
 */
#define VTM_SQUEUE_FOR_EACH(NAME, NODE_PTR)                      \
	for (NODE_PTR = NAME.head; NODE_PTR != NULL;                 \
		NODE_PTR = NODE_PTR->next)

/**
 * Iterates through the queue and allows possible modification.
 *
 * @param NAME the target queue variable
 * @param[out] NODE_PTR will point to current item
 * @param[out] PREV_PTR will point to previous item, can be NULL
 * @param[out] NEXT_PTR will point to next item, can be NULL
 */
#define VTM_SQUEUE_FOR_EACH_MOD(NAME,NODE_PTR,PREV_PTR,NEXT_PTR) \
	for (NODE_PTR = NAME.head,                                   \
		PREV_PTR = NULL,                                         \
		NEXT_PTR = NODE_PTR ? NODE_PTR->next : NULL;             \
		NODE_PTR != NULL;                                        \
		PREV_PTR = NODE_PTR,                                     \
		NODE_PTR = NEXT_PTR,                                     \
		NEXT_PTR = NEXT_PTR ? NEXT_PTR->next : NULL)

/**
 * Removes all items from the queue.
 *
 * @param NAME the target queue variable
 * @param NODE_TYPE name of the struct whose instances are stored
 *        as items in the queue
 * @param FUNC function pointer that is called with each
 *        removed element
 */
#define VTM_SQUEUE_CLEAR(NAME, NODE_TYPE, FUNC)                  \
	do {                                                         \
		NODE_TYPE *node;                                         \
		NODE_TYPE *prev;                                         \
		NODE_TYPE *next;                                         \
		VTM_UNUSED(prev);                                        \
		VTM_SQUEUE_FOR_EACH_MOD(NAME, node, prev, next) {        \
			FUNC(node);                                          \
		}                                                        \
		NAME.head = NULL;                                        \
		NAME.tail = NULL;                                        \
	} while (0)

#endif /* VTM_CORE_SQUEUE_H_ */
