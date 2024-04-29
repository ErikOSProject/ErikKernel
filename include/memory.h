#ifndef _MEMORY_H
#define _MEMORY_H

#define EFI_CONVENTIONAL_MEMORY 7

typedef struct {
	uintptr_t base;
	size_t length;
	uint8_t *bitmap;
} memory;

void *memset(void *destination, int c, size_t num);
intptr_t find_free_frames(size_t n);
intptr_t set_frame_lock(uintptr_t frame, size_t n, bool lock);
void page_frame_allocator_init(BootInfo *boot_info);

#endif //_MEMORY_H
