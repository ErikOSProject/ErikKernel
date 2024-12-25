/**
 * @file heap.h
 * @brief Dynamic memory allocation functions.
 *
 * This file contains the declarations for the dynamic memory allocation functions.
 */

#ifndef _HEAP_H
#define _HEAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <erikboot.h>

typedef struct _heap_block heap_block;
struct _heap_block {
	bool used;
	size_t size;
	heap_block *previous;
	heap_block *next;
};

void heap_init(BootInfo *boot_info);
void *malloc(size_t size);
void free(void *ptr);

#endif //_HEAP_H
