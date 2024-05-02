#include <debug.h>
#include <erikboot.h>
#include <heap.h>
#include <memory.h>
#include <paging.h>

uintptr_t heap_start = 0;
uintptr_t heap_end = 0;
heap_block *first_block = NULL;
heap_block *last_block = NULL;

extern char _kernel_end;

void heap_calculate_virtual_start(BootInfo *boot_info)
{
	if (boot_info->InitrdBase)
		heap_start = (((uintptr_t)boot_info->InitrdBase +
			       boot_info->InitrdSize) &
			      ~0xFFF) +
			     0x1000;
	else if (boot_info->FBBase)
		heap_start =
			(((uintptr_t)boot_info->FBBase + boot_info->FBSize) &
			 ~0xFFF) +
			0x1000;
	else
		heap_start = ((uintptr_t)&_kernel_end & ~0xFFF) + 0x1000;
	heap_end = heap_start + 0x1000;
}

void heap_split_block(heap_block *first, size_t size)
{
	heap_block *second =
		(heap_block *)((uintptr_t)first + sizeof(heap_block) + size);
	size_t second_size = first->size - size - sizeof(heap_block);

	second->used = false;
	second->previous = first;
	second->next = first->next;
	second->size = second_size;
	first->next = second;
	first->size = size;
	if (second->next)
		second->next->previous = second;

	if (last_block == first)
		last_block = second;
}

void heap_merge_blocks(heap_block *first, heap_block *second)
{
	if (second->next)
		second->next->previous = first;
	first->next = second->next;
	first->size += second->size + sizeof(heap_block);

	if (last_block == second)
		last_block = first;
}

bool expand_heap(void)
{
	uintptr_t page = find_free_frames(1);
	if (!page)
		return false;

	set_frame_lock(page, 1, true);
	paging_map_page(tables, heap_end, page, P_KERNEL_WRITE);
	heap_block *block = (heap_block *)heap_end;
	heap_end += PAGE_SIZE;
	block->size = PAGE_SIZE - sizeof(heap_block);
	block->previous = last_block;
	block->next = NULL;
	block->used = false;

	if (last_block && !last_block->used)
		heap_merge_blocks(last_block, block);
	else {
		last_block->next = block;
		last_block = block;
	}
	return true;
}

void heap_init(BootInfo *boot_info)
{
	heap_calculate_virtual_start(boot_info);
	if (!heap_start)
		return;

	uintptr_t first_page = find_free_frames(1);
	if (!first_page)
		return;

	set_frame_lock(first_page, 1, true);
	paging_map_page(tables, heap_start, first_page, P_KERNEL_WRITE);
	last_block = first_block = (heap_block *)heap_start;
	first_block->size = PAGE_SIZE - sizeof(heap_block);
	first_block->previous = NULL;
	first_block->next = NULL;
	first_block->used = false;
}

heap_block *do_malloc(size_t size)
{
	heap_block *i = first_block;
	while (i) {
		if (!i->used && i->size >= size) {
			if (i->size > size + 2 * sizeof(heap_block))
				heap_split_block(i, size);
			i->used = true;
			return i;
		}
		i = i->next;
	}
	return NULL;
}

void *malloc(size_t size)
{
	heap_block *i = NULL;
	while (true) {
		if ((i = do_malloc(size)))
			return (uint8_t *)i + sizeof(heap_block);
		if (!expand_heap())
			return NULL;
	}
}

void free(void *ptr)
{
	heap_block *i = (heap_block *)((uintptr_t)ptr - sizeof(heap_block));
	i->used = false;

	if (i->next && !i->next->used)
		heap_merge_blocks(i, i->next);
	if (i->previous && !i->previous->used)
		heap_merge_blocks(i->previous, i);
}
