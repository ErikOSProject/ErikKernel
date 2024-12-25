/**
 * @file heap.c
 * @brief Implementation of the heap memory allocator.
 *
 * This file contains the implementation of the heap memory allocator used
 * within the ErikKernel project. The heap allocator provides functions for
 * allocating and freeing memory on the heap, as well as initializing the heap
 * memory region.
 */

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

/**
 * @brief Calculates the virtual start address for the heap.
 *
 * This function determines the starting virtual address for the heap based on
 * the provided boot information.
 *
 * @param boot_info A pointer to the BootInfo structure containing boot-related information.
 */
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

/**
 * @brief Splits a heap block into two blocks.
 *
 * This function takes a pointer to the first block and a size, and splits the 
 * first block into two blocks. The first block will have the specified size, 
 * and the second block will contain the remaining space.
 *
 * @param first Pointer to the first heap block to be split.
 * @param size The size of the first block after the split.
 */
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

/**
 * @brief Merges two adjacent heap blocks into a single block.
 *
 * This function takes two adjacent heap blocks and merges them into a single
 * block. The first block should be the one that comes before the second block
 * in memory. After merging, the first block will encompass the memory of both
 * blocks, and the second block will no longer be valid.
 *
 * @param first Pointer to the first heap block.
 * @param second Pointer to the second heap block.
 */
void heap_merge_blocks(heap_block *first, heap_block *second)
{
	if (second->next)
		second->next->previous = first;
	first->next = second->next;
	first->size += second->size + sizeof(heap_block);

	if (last_block == second)
		last_block = first;
}

/**
 * @brief Expands the heap to accommodate more memory.
 *
 * This function attempts to increase the size of the heap to provide
 * additional memory for allocation. It does this by mapping a new page
 * of memory and adding it to the heap as a new block.
 *
 * @return true if the heap was successfully expanded, false otherwise.
 */
bool expand_heap(void)
{
	intptr_t page = find_free_frames(1);
	if (page == -1)
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

/**
 * @brief Initializes the heap memory management system.
 *
 * This function sets up the heap using the provided boot information.
 *
 * @param boot_info A pointer to the BootInfo structure containing
 *                  information about the system's memory layout.
 */
void heap_init(BootInfo *boot_info)
{
	heap_calculate_virtual_start(boot_info);
	if (!heap_start)
		return;

	intptr_t first_page = find_free_frames(1);
	if (first_page == -1)
		return;

	set_frame_lock(first_page, 1, true);
	paging_map_page(tables, heap_start, first_page, P_KERNEL_WRITE);
	last_block = first_block = (heap_block *)heap_start;
	first_block->size = PAGE_SIZE - sizeof(heap_block);
	first_block->previous = NULL;
	first_block->next = NULL;
	first_block->used = false;
}

/**
 * @brief Allocates a block of memory on the heap. (helper function)
 *
 * This function allocates a block of memory of the specified size on the heap.
 * Returns NULL if the allocation fails. Use malloc() instead of this function to
 * allocate memory allowing the heap to expand if necessary.
 *
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block, or NULL if the allocation fails.
 */
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

/**
 * @brief Allocates a block of memory on the heap.
 *
 * This function allocates a block of memory of the specified size on the heap.
 * If the allocation fails, it attempts to expand the heap to accommodate the
 * requested memory size.
 *
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block, or NULL if the allocation fails.
 */
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

/**
 * @brief Frees the memory space pointed to by ptr, which must have been returned by a previous call to malloc().
 *
 * @param ptr Pointer to the memory to be freed.
 */
void free(void *ptr)
{
	heap_block *i = (heap_block *)((uintptr_t)ptr - sizeof(heap_block));
	if ((uintptr_t)i < heap_start || (uintptr_t)i >= heap_end)
		return;
	i->used = false;

	if (i->next && !i->next->used)
		heap_merge_blocks(i, i->next);
	if (i->previous && !i->previous->used)
		heap_merge_blocks(i->previous, i);
}
