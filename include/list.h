/**
 * @file list.h
 * @brief Doubly linked list implementation.
 *
 * This file contains the declarations for a doubly linked list data structure.
 * It allows for the creation, manipulation, and destruction of linked lists.
 */
#ifndef _LIST_H
#define _LIST_H

#include <erikboot.h>

struct node {
	struct node *prev;
	struct node *next;
	void *value;
};

struct list {
	struct node *head;
	struct node *tail;
	size_t length;
};

struct list *list_create(void);
void list_destroy(struct list *list);
void list_append(struct list *list, struct node *node);
struct node *list_insert_tail(struct list *list, void *value);
void list_prepend(struct list *list, struct node *node);
struct node *list_insert_head(struct list *list, void *value);
void list_delete(struct list *list, struct node *node);
struct node *list_find(struct list *list, void *value);
struct node *list_pop(struct list *list);
struct node *list_dequeue(struct list *list);
struct node *list_index(struct list *list, size_t index);
struct list *list_copy(struct list *list);
void list_append_after(struct list *list, struct node *prev, struct node *node);
void list_append_before(struct list *list, struct node *next,
			struct node *node);
struct node *list_insert_after(struct list *list, struct node *prev,
			       void *value);
struct node *list_insert_before(struct list *list, struct node *next,
				void *value);
void list_merge(struct list *list1, struct list *list2);

#endif //_LIST_H
