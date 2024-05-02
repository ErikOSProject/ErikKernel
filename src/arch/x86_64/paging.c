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

void get_pml4(void)
{
	if (!tables)
		asm volatile("movq %%cr3, %0" : "=r"(tables));
}

uint64_t paging_flags_to_arch(uint64_t flags)
{
	uint64_t arch_flags = P_X64_PRESENT;
	if (flags & P_USER)
		arch_flags |= P_X64_USER;
	if (flags & P_WRITE)
		arch_flags |= P_X64_WRITE;
	return arch_flags;
}

uint64_t *paging_create_table(void)
{
	uintptr_t table = find_free_frames(1);
	if (!table)
		return NULL;
	set_frame_lock(table, 1, true);
	memset((char *)table, 0, PAGE_SIZE);
	return (uint64_t *)table;
}

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
