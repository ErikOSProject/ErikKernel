/**
 * @file paging.c
 * @brief Paging implementation for x86_64 architecture.
 *
 * This file contains the implementation of the paging functions for the x86_64 architecture.
 * It allows for the creation of page tables, mapping and unmapping of virtual memory to physical memory,
 * and setting up the page tables for the system.
 */

#include <erikboot.h>
#include <memory.h>

#include <arch/x86_64/paging.h>

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
	ap_pml4 = (uintptr_t)tables;
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
	if (flags & P_COW)
		arch_flags |= P_X64_COW;
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
	intptr_t table = find_free_frames(1);
	if (table == -1)
		return NULL;
	set_frame_lock(table, 1, true);
	memset((char *)table, 0, PAGE_SIZE);
	return (uint64_t *)table;
}

/**
 * @brief Maps a virtual address to a physical address in the page tables.
 *
 * @param pml4 A pointer to the top-level page table.
 * @param vaddr The virtual address to be mapped.
 * @param paddr The physical address to map to the virtual address.
 * @param flags The paging flags to be set for the mapping.
 */
void paging_map_page(uint64_t *pml4, uintptr_t vaddr, uintptr_t paddr,
		     uint64_t flags)
{
	uint64_t pml4_index = PML4_INDEX(vaddr);
	uint64_t pdpt_index = PDPT_INDEX(vaddr);
	uint64_t pd_index = PD_INDEX(vaddr);
	uint64_t pt_index = PT_INDEX(vaddr);

	uint64_t *pdpt = (uint64_t *)(pml4[pml4_index] & ~0xFFF);
	if (!pdpt || !(pml4[pml4_index] & P_X64_PRESENT)) {
		pdpt = paging_create_table();
		pml4[pml4_index] = (uint64_t)pdpt | TABLE_DEFAULT;
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
	frame_ref_inc(paddr & ~0xFFF);
}

/**
 * @brief Unmaps a page from the page tables.
 *
 * This function removes the mapping of a virtual address from the page tables.
 *
 * @param pml4 A pointer to the top-level page table.
 * @param vaddr The virtual address to unmap.
 */
void paging_unmap_page(uint64_t *pml4, uintptr_t vaddr)
{
	uint64_t pml4_index = PML4_INDEX(vaddr);
	uint64_t pdpt_index = PDPT_INDEX(vaddr);
	uint64_t pd_index = PD_INDEX(vaddr);
	uint64_t pt_index = PT_INDEX(vaddr);

	if (!(pml4[pml4_index] & P_X64_PRESENT))
		return;
	uint64_t *pdpt = (uint64_t *)(pml4[pml4_index] & ~0xFFF);
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
	uintptr_t paddr = pt[pt_index] & ~0xFFF;
	pt[pt_index] = 0;
	frame_ref_dec(paddr);

	asm volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
}

/**
 * @brief Clones the higher half of the page tables.
 *
 * This function clones the higher half of the page tables to the lower half.
 *
 * @param src A pointer to the source page table.
 * @param dst A pointer to the destination page table.
 */
void paging_clone_higher_half(uint64_t *src, uint64_t *dst)
{
	uint64_t *dst_pdpt = paging_create_table();
	dst[0x1ff] = (uint64_t)dst_pdpt | TABLE_DEFAULT;
	uint64_t *dst_pd = paging_create_table();
	dst_pdpt[0x1ff] = (uint64_t)dst_pd | TABLE_DEFAULT;

	uint64_t *src_pdpt = (uint64_t *)(src[0x1ff] & ~0xFFF);
	uint64_t *src_pd = (uint64_t *)(src_pdpt[0x1ff] & ~0xFFF);

	for (int i = 448; i < 512; i++)
		dst_pd[i] = src_pd[i];
}

/**
 * @brief Sets the current page table.
 *
 * This function sets the current page table for the system.
 *
 * @param pml4 A pointer to the page table to set as the current table.
 */
void paging_set_current(uint64_t *pml4)
{
	asm volatile("movq %0, %%cr3" ::"r"(pml4) : "memory");
}
