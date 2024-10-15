/**
 * @file paging.c
 * @brief Paging implementation for x86_64 architecture.
 *
 * This file contains the implementation of the paging functions for the x86_64 architecture.
 * It allows for the creation of page tables, mapping and unmapping of virtual memory to physical memory,
 * and setting up the page tables for the system.
 */

#include <debug.h>
#include <erikboot.h>
#include <memory.h>
#include <paging.h>

#define PML4_INDEX(x) (((x) >> 39) & 0x1FF)
#define PDPT_INDEX(x) (((x) >> 30) & 0x1FF)
#define PD_INDEX(x) (((x) >> 21) & 0x1FF)
#define PT_INDEX(x) (((x) >> 12) & 0x1FF)

#define P_X64_PRESENT (1 << 0)
#define P_X64_WRITE (1 << 1)
#define P_X64_USER (1 << 2)

#define TABLE_DEFAULT (P_X64_PRESENT | P_X64_WRITE | P_X64_USER)

uint64_t *tables = NULL;

/**
 * @brief Retrieves the current Page Map Level 4 (PML4) table.
 *
 * This function is responsible for obtaining the address of the current
 * PML4 table, which is the top-level page table in the x86_64 architecture's
 * 4-level paging hierarchy.
 *
 * @return void
 */
void get_pml4(void)
{
	if (!tables)
		asm volatile("movq %%cr3, %0" : "=r"(tables));
}

/**
 * @brief Converts generic paging flags to architecture-specific flags.
 *
 * This function takes a set of generic paging flags and converts them
 * to the corresponding architecture-specific flags for the x86_64 architecture.
 *
 * @param flags The generic paging flags to be converted.
 * @return The architecture-specific paging flags.
 */
uint64_t paging_flags_to_arch(uint64_t flags)
{
	uint64_t arch_flags = P_X64_PRESENT;
	if (flags & P_USER)
		arch_flags |= P_X64_USER;
	if (flags & P_WRITE)
		arch_flags |= P_X64_WRITE;
	return arch_flags;
}

/**
 * @brief Creates a new page table.
 *
 * This function allocates and initializes a new page table for the x86_64 architecture.
 *
 * @return A pointer to the newly created paging table.
 */
uint64_t *paging_create_table(void)
{
	uintptr_t table = find_free_frames(1);
	if (!table)
		return NULL;
	set_frame_lock(table, 1, true);
	memset((char *)table, 0, PAGE_SIZE);
	return (uint64_t *)table;
}

/**
 * @brief Maps a virtual address to a physical address in the page tables.
 *
 * @param tables A pointer to the top-level page table.
 * @param vaddr The virtual address to be mapped.
 * @param paddr The physical address to map to the virtual address.
 * @param flags The paging flags to be set for the mapping.
 */
void paging_map_page(uint64_t *tables, uintptr_t vaddr, uintptr_t paddr,
		     uint64_t flags)
{
	uint64_t pml4_index = PML4_INDEX(vaddr);
	uint64_t pdpt_index = PDPT_INDEX(vaddr);
	uint64_t pd_index = PD_INDEX(vaddr);
	uint64_t pt_index = PT_INDEX(vaddr);

	uint64_t *pdpt = (uint64_t *)(tables[pml4_index] & ~0xFFF);
	if (!pdpt || !(tables[pml4_index] & P_X64_PRESENT)) {
		pdpt = paging_create_table();
		tables[pml4_index] = (uint64_t)pdpt | TABLE_DEFAULT;
	}

	uint64_t *pd = (uint64_t *)(pdpt[pdpt_index] & ~0xFFF);
	if (!pd || !(pdpt[pdpt_index] & P_X64_PRESENT)) {
		pd = paging_create_table();
		pdpt[pdpt_index] = (uint64_t)pd | TABLE_DEFAULT;
	}

	uint64_t *pt = (uint64_t *)(pd[pd_index] & ~0xFFF);
	if (!pt || !(pd[pd_index] & P_X64_PRESENT)) {
		pt = paging_create_table();
		pd[pd_index] = (uint64_t)pt | TABLE_DEFAULT;
	}

	uint64_t arch_flags = paging_flags_to_arch(flags);
	pt[pt_index] = (paddr & ~0xFFF) | arch_flags;
}

/**
 * @brief Unmaps a page from the page tables.
 *
 * This function removes the mapping of a virtual address from the page tables.
 *
 * @param tables A pointer to the top-level page table.
 * @param vaddr The virtual address to unmap.
 */
void paging_unmap_page(uint64_t *tables, uintptr_t vaddr)
{
	uint64_t pml4_index = PML4_INDEX(vaddr);
	uint64_t pdpt_index = PDPT_INDEX(vaddr);
	uint64_t pd_index = PD_INDEX(vaddr);
	uint64_t pt_index = PT_INDEX(vaddr);

	if (!(tables[pml4_index] & P_X64_PRESENT))
		return;
	uint64_t *pdpt = (uint64_t *)(tables[pml4_index] & ~0xFFF);
	if (!pdpt)
		return;

	if (!(pdpt[pdpt_index] & P_X64_PRESENT))
		return;
	uint64_t *pd = (uint64_t *)(pdpt[pdpt_index] & ~0xFFF);
	if (!pd)
		return;

	if (!(pd[pd_index] & P_X64_PRESENT))
		return;
	uint64_t *pt = (uint64_t *)(pd[pd_index] & ~0xFFF);
	if (!pt)
		return;

	if (!(pt[pt_index] & P_X64_PRESENT))
		return;
	pt[pt_index] = 0;

	asm volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
}
