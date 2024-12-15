/**
 * @file memory.h
 * @brief Memory management functions.
 *
 * This file contains the declarations for the memory management functions.
 */

#ifndef _MEMORY_H
#define _MEMORY_H

#define PAGE_SIZE 4096

typedef struct {
	uintptr_t base;
	size_t length;
	uint8_t *bitmap;
} memory;

void *memset(void *destination, int c, size_t num);
size_t strlen(const char *str);
char *memcpy(char *destination, const char *source, size_t n);
char *strcpy(char *destination, const char *source);
char *strcat(char *destination, const char *source);
int memcmp(const void *str1, const void *str2, size_t n);
int strcmp(const char *str1, const char *str2);
char *strtok(char *str, const char *delimiters);
intptr_t find_free_frames(size_t n);
intptr_t set_frame_lock(uintptr_t frame, size_t n, bool lock);
void page_frame_allocator_init(BootInfo *boot_info);

#endif //_MEMORY_H
