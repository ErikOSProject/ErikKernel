/**
 * @file list.c
 * @brief Doubly linked list implementation.
 *
 * This file contains the implementation of a doubly linked list data structure.
 * It allows for the creation, manipulation, and destruction of linked lists.
 */
#include <list.h>
#include <heap.h>

/**
 * @brief Creates a new linked list.
 *
 * This function creates a new linked list and returns a pointer to it.
 *
 * @return A pointer to the newly created linked list.
 */
struct list *list_create(void)
{
	struct list *list = malloc(sizeof(struct list));
	list->head = NULL;
	list->tail = NULL;
	list->length = 0;
	return list;
}

/**
 * @brief Destroys a linked list.
 *
 * This function destroys a linked list and frees all memory associated with it.
 *
 * @param list A pointer to the linked list to destroy.
 */
void list_destroy(struct list *list)
{
	struct node *node = list->head;
	while (node) {
		struct node *next = node->next;
		free(node);
		node = next;
	}
	free(list);
}

/**
 * @brief Appends a node to the end of a linked list.
 *
 * This function appends a node to the end of a linked list.
 *
 * @param list A pointer to the linked list.
 * @param node A pointer to the node to append.
 */
void list_append(struct list *list, struct node *node)
{
	if (list->tail) {
		list->tail->next = node;
		node->prev = list->tail;
		node->next = NULL;
		list->tail = node;
	} else {
		list->head = node;
		list->tail = node;
		node->prev = NULL;
		node->next = NULL;
	}
	list->length++;
}

/**
 * @brief Inserts a node at the end of a linked list.
 *
 * This function inserts a node at the end of a linked list.
 *
 * @param list A pointer to the linked list.
 * @param value A pointer to the value to insert.
 * @return A pointer to the newly inserted node.
 */
struct node *list_insert_tail(struct list *list, void *value)
{
	struct node *node = malloc(sizeof(struct node));
	node->value = value;
	list_append(list, node);
	return node;
}

/**
 * @brief Prepends a node to the beginning of a linked list.
 *
 * This function prepends a node to the beginning of a linked list.
 *
 * @param list A pointer to the linked list.
 * @param node A pointer to the node to prepend.
 */
void list_prepend(struct list *list, struct node *node)
{
	if (list->head) {
		list->head->prev = node;
		node->next = list->head;
		node->prev = NULL;
		list->head = node;
	} else {
		list->head = node;
		list->tail = node;
		node->prev = NULL;
		node->next = NULL;
	}
	list->length++;
}

/**
 * @brief Inserts a node at the beginning of a linked list.
 *
 * This function inserts a node at the beginning of a linked list.
 *
 * @param list A pointer to the linked list.
 * @param value A pointer to the value to insert.
 * @return A pointer to the newly inserted node.
 */
struct node *list_insert_head(struct list *list, void *value)
{
	struct node *node = malloc(sizeof(struct node));
	node->value = value;
	list_prepend(list, node);
	return node;
}

/**
 * @brief Deletes a node from a linked list.
 *
 * This function deletes a node from a linked list.
 *
 * @param list A pointer to the linked list.
 * @param node A pointer to the node to delete.
 */
void list_delete(struct list *list, struct node *node)
{
	if (node->prev && node->next) {
		node->prev->next = node->next;
		node->next->prev = node->prev;
	} else if (node->prev) {
		node->prev->next = NULL;
	} else if (node->next) {
		node->next->prev = NULL;
	}

	if (list->head == node) {
		list->head = node->next;
	} else if (list->tail == node) {
		list->tail = node->prev;
	}

	free(node);
}

/**
 * @brief Finds a node in a linked list.
 *
 * This function finds a node in a linked list with the specified value.
 *
 * @param list A pointer to the linked list.
 * @param value A pointer to the value to find.
 * @return A pointer to the node with the specified value, or NULL if not found.
 */
struct node *list_find(struct list *list, void *value)
{
	struct node *node = list->head;
	while (node) {
		if (node->value == value) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

/**
 * @brief Pops a node from the beginning of a linked list.
 *
 * This function pops a node from the beginning of a linked list.
 *
 * @param list A pointer to the linked list.
 * @return A pointer to the popped node.
 */
struct node *list_pop(struct list *list)
{
	struct node *node = list->head;
	if (node) {
		list->head = node->next;
		if (list->head) {
			list->head->prev = NULL;
		} else {
			list->tail = NULL;
		}
		list->length--;
	}
	return node;
}

/**
 * @brief Dequeues a node from the end of a linked list.
 *
 * This function dequeues a node from the end of a linked list.
 *
 * @param list A pointer to the linked list.
 * @return A pointer to the dequeued node.
 */
struct node *list_dequeue(struct list *list)
{
	struct node *node = list->tail;
	if (node) {
		list->tail = node->prev;
		if (list->tail) {
			list->tail->next = NULL;
		} else {
			list->head = NULL;
		}
		list->length--;
	}
	return node;
}

/**
 * @brief Gets the node at the specified index in a linked list.
 *
 * This function gets the node at the specified index in a linked list.
 *
 * @param list A pointer to the linked list.
 * @param index The index of the node to get.
 * @return A pointer to the node at the specified index, or NULL if not found.
 */
struct node *list_index(struct list *list, size_t index)
{
	struct node *node = list->head;
	for (size_t i = 0; i < index; i++) {
		if (!node) {
			return NULL;
		}
		node = node->next;
	}
	return node;
}

/**
 * @brief Copies a linked list.
 *
 * This function copies a linked list and returns a pointer to the new list.
 *
 * @param list A pointer to the linked list to copy.
 * @return A pointer to the newly copied linked list.
 */
struct list *list_copy(struct list *list)
{
	struct list *new_list = list_create();
	struct node *node = list->head;
	while (node) {
		list_append(new_list, node->value);
		node = node->next;
	}
	return new_list;
}

/**
 * @brief Appends a node after a specified node in a linked list.
 *
 * This function appends a node after a specified node in a linked list.
 *
 * @param list A pointer to the linked list.
 * @param prev A pointer to the node after which to append.
 * @param node A pointer to the node to append.
 */
void list_append_after(struct list *list, struct node *prev, struct node *node)
{
	if (prev) {
		node->prev = prev;
		node->next = prev->next;
		prev->next = node;
		if (node->next) {
			node->next->prev = node;
		} else {
			list->tail = node;
		}
		list->length++;
	} else {
		list_prepend(list, node);
	}
}

/**
 * @brief Appends a node before a specified node in a linked list.
 *
 * This function appends a node before a specified node in a linked list.
 *
 * @param list A pointer to the linked list.
 * @param next A pointer to the node before which to append.
 * @param node A pointer to the node to append.
 */
void list_append_before(struct list *list, struct node *next, struct node *node)
{
	if (next) {
		node->next = next;
		node->prev = next->prev;
		next->prev = node;
		if (node->prev) {
			node->prev->next = node;
		} else {
			list->head = node;
		}
		list->length++;
	} else {
		list_append(list, node);
	}
}

/**
 * @brief Inserts a node after a specified node in a linked list.
 *
 * This function inserts a node after a specified node in a linked list.
 *
 * @param list A pointer to the linked list.
 * @param prev A pointer to the node after which to insert.
 * @param value A pointer to the value to insert.
 * @return A pointer to the newly inserted node.
 */
struct node *list_insert_after(struct list *list, struct node *prev,
			       void *value)
{
	struct node *node = malloc(sizeof(struct node));
	node->value = value;
	list_append_after(list, prev, node);
	return node;
}

/**
 * @brief Inserts a node before a specified node in a linked list.
 *
 * This function inserts a node before a specified node in a linked list.
 *
 * @param list A pointer to the linked list.
 * @param next A pointer to the node before which to insert.
 * @param value A pointer to the value to insert.
 * @return A pointer to the newly inserted node.
 */
struct node *list_insert_before(struct list *list, struct node *next,
				void *value)
{
	struct node *node = malloc(sizeof(struct node));
	node->value = value;
	list_append_before(list, next, node);
	return node;
}

/**
 * @brief Merges two linked lists.
 *
 * This function merges two linked lists by appending the second list to the first list.
 *
 * @param list1 A pointer to the first linked list.
 * @param list2 A pointer to the second linked list.
 */
void list_merge(struct list *list1, struct list *list2)
{
	if (list1->tail) {
		list1->tail->next = list2->head;
		list2->head->prev = list1->tail;
		list1->tail = list2->tail;
		list1->length += list2->length;
	} else {
		list1->head = list2->head;
		list1->tail = list2->tail;
		list1->length = list2->length;
	}
	free(list2);
}
