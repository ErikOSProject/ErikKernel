/**
 * @file paging.c
 * @brief Paging implementation for AArch64 architecture.
 *
 * This file contains the implementation of the paging functions for the AArch64 architecture.
 * It allows for the creation of page tables, mapping and unmapping of virtual memory to physical memory,
 * and setting up the page tables for the system.
 */

#include <debug.h>
#include <erikboot.h>
#include <memory.h>
#include <paging.h>

#define PGD_INDEX(x) (((x) >> 39) & 0x1FF)
#define PUD_INDEX(x) (((x) >> 30) & 0x1FF)
#define PMD_INDEX(x) (((x) >> 21) & 0x1FF)
#define PT_INDEX(x) (((x) >> 12) & 0x1FF)

#define P_AARCH64_AF (1 << 8)
#define P_AARCH64_RO (1 << 5)
#define P_AARCH64_USER (1 << 4)

typedef struct {
	union {
		struct {
			uint64_t present : 1;
			uint64_t table_block : 1;
			uint64_t attributes_low : 10;
			uint64_t address : 36;
			uint64_t reserved : 4;
			uint64_t attributes_high : 12;
		};
		uint64_t value;
	};
} PTE;

uint64_t *tables = NULL;
uint64_t *ttbr1_el1 = NULL;

/**
 * @brief Retrieves the value of the Translation Table Base Register 0 (TTBR0).
 *
 * This function is used to obtain the current value of TTBR0, which holds the 
 * base address of the translation table for the lower half of the address space 
 * in AArch64 architecture.
 */
void get_ttbr0(void)
{
	if (!tables)
		asm volatile("mrs %0, ttbr0_el1;" : "=r"(tables));
}

/**
 * @brief Retrieves the value of the Translation Table Base Register 1 (TTBR1).
 *
 * This function is used to obtain the current value of TTBR1, which holds the 
 * base address of the translation table for the higher half of the address space 
 * in AArch64 architecture.
 *
 * @return The value of TTBR1.
 */
void get_ttbr1(void)
{
	if (!ttbr1_el1)
		asm volatile("mrs %0, ttbr1_el1;" : "=r"(ttbr1_el1));
}

/**
 * @brief Converts generic paging flags to architecture-specific flags for AArch64.
 *
 * @param flags The generic paging flags to be converted.
 * @return The architecture-specific paging flags.
 */
uint64_t paging_flags_to_arch(uint64_t flags)
{
	uint64_t arch_flags = P_AARCH64_AF;
	if (flags & P_USER)
		arch_flags |= P_AARCH64_USER;
	if (!(flags & P_WRITE))
		arch_flags |= P_AARCH64_RO;
	return arch_flags;
}

/**
 * @brief Creates a new page table.
 *
 * This function allocates and initializes a new page table for the AArch64 architecture.
 *
 * @return A pointer to the newly created page table.
 */
uint64_t *paging_create_table(void)
{
	intptr_t table = find_free_frames(1);
	if (table != -1)
		return NULL;
	set_frame_lock(table, 1, true);
	memset((char *)table, 0, PAGE_SIZE);
	return (uint64_t *)table;
}

/**
 * @brief Maps a virtual address to a physical address in the page tables.
 *
 * @param tables A pointer to the page tables.
 * @param vaddr The virtual address to be mapped.
 * @param paddr The physical address to map to the virtual address.
 * @param flags The paging flags to be set for the mapping.
 */
void paging_map_page(uint64_t *tables, uintptr_t vaddr, uintptr_t paddr,
		     uint64_t flags)
{
	uint64_t pgd_index = PGD_INDEX(vaddr);
	uint64_t pud_index = PUD_INDEX(vaddr);
	uint64_t pmd_index = PMD_INDEX(vaddr);
	uint64_t pt_index = PT_INDEX(vaddr);

	PTE *pmd;
	if (vaddr < 0xfffffffff8000000) {
		PTE *pgd = (PTE *)tables;

		PTE *pud;
		if (pgd[pgd_index].present)
			pud = (PTE *)(pgd[pgd_index].address << 12);
		else {
			pud = (PTE *)paging_create_table();
			pgd[pgd_index].present = true;
			pgd[pgd_index].table_block = true;
			pgd[pgd_index].address = (uintptr_t)pud >> 12;
		}

		if (pud[pud_index].present)
			pmd = (PTE *)(pud[pud_index].address << 12);
		else {
			pmd = (PTE *)paging_create_table();
			pud[pud_index].present = true;
			pud[pud_index].table_block = true;
			pud[pud_index].address = (uintptr_t)pud >> 12;
		}
	} else {
		pmd_index &= 0x3f;
		pmd = (PTE *)ttbr1_el1;
	}

	PTE *pt;
	if (pmd[pmd_index].present) {
		pt = (PTE *)(pmd[pmd_index].address << 12);
	} else {
		pt = (PTE *)paging_create_table();
		pmd[pmd_index].present = true;
		pmd[pmd_index].table_block = true;
		pmd[pmd_index].address = (uintptr_t)pt >> 12;
	}

	pt[pt_index].present = true;
	pt[pt_index].table_block = true;
	pt[pt_index].attributes_low = paging_flags_to_arch(flags);
	pt[pt_index].address = paddr >> 12;

	asm volatile("isb;");
}

/**
 * @brief Unmaps a page from the page tables.
 *
 * This function removes the mapping of a virtual address from the page tables.
 *
 * @param tables Pointer to the page tables.
 * @param vaddr The virtual address to unmap.
 */
void paging_unmap_page(uint64_t *tables, uintptr_t vaddr)
{
	uint64_t pgd_index = PGD_INDEX(vaddr);
	uint64_t pud_index = PUD_INDEX(vaddr);
	uint64_t pmd_index = PMD_INDEX(vaddr);
	uint64_t pt_index = PT_INDEX(vaddr);

	PTE *pmd;
	if (vaddr < 0xfffffffff8000000) {
		PTE *pgd = (PTE *)tables;

		PTE *pud = (PTE *)(pgd[pgd_index].address << 12);
		if (!pgd[pgd_index].present)
			return;

		pmd = (PTE *)(pud[pud_index].address << 12);
		if (!pud[pud_index].present)
			return;
	} else {
		pmd_index &= 0x3f;
		pmd = (PTE *)ttbr1_el1;
	}

	PTE *pt = (PTE *)(pmd[pmd_index].address << 12);
	if (!pmd[pmd_index].present)
		return;

	pt[pt_index].present = false;
	asm volatile("isb;");
}
