/**
 * @file gdt.c
 * @brief Global Descriptor Table implementation for x86_64 architecture.
 *
 * This file contains the implementation of the Global Descriptor Table for the x86_64 architecture.
 * It allows for the initialization of the GDT and the setting of segment descriptors.
 */

#include <heap.h>
#include <debug.h>

#include <arch/x86_64/apic.h>
#include <arch/x86_64/gdt.h>

gdt *_gdt;

/**
 * @brief Loads the Global Descriptor Table (GDT) for the specified core.
 *
 * This function loads the Global Descriptor Table (GDT) for the specified core.
 *
 * @param id The ID of the core for which to load the GDT.
 */
void load_gdt(size_t id)
{
	asm volatile("lgdt %0;"
		     "pushq $0x8;"
		     "leaq 1f, %%rax;"
		     "pushq %%rax;"
		     "lretq;"
		     "1:"
		     "movw $0x10, %%ax;"
		     "movw %%ax, %%ds;"
		     "movw %%ax, %%es;"
		     "movw %%ax, %%fs;"
		     "movw %%ax, %%gs;"
		     "movw %%ax, %%ss;"
		     "movw $0x30, %%ax;"
		     "ltr %%ax;"
		     :
		     : "m"(_gdt[id].gdtr)
		     : "rax", "memory");
}

/**
 * @brief Writes a segment descriptor to the GDT.
 *
 * This function writes a segment descriptor to the Global Descriptor Table (GDT).
 *
 * @param desc The segment descriptor to write.
 * @param base The base address of the segment.
 * @param limit The limit of the segment.
 * @param access The access flags for the segment.
 * @param flags The flags for the segment.
 */
void write_segment_descriptor(segment_descriptor *desc, uint32_t base,
			      uint32_t limit, uint8_t access, uint8_t flags)
{
	desc->limit_low = limit & 0xFFFF;
	desc->base_low = base & 0xFFFF;
	desc->base_mid = (base >> 16) & 0xFF;
	desc->access = access;
	desc->limit_high = (limit >> 16) & 0xF;
	desc->flags = flags & 0xF;
	desc->base_high = (base >> 24) & 0xFF;
}

/**
 * @brief Writes a Task State Segment (TSS) segment descriptor to the GDT.
 *
 * This function writes a Task State Segment (TSS) segment descriptor to the Global Descriptor Table (GDT).
 *
 * @param desc The segment descriptor to write.
 * @param tss The TSS to write.
 */
void write_tss_segment_descriptor(segment_descriptor *desc, tss *tss)
{
	desc[0].access = 0x89;
	desc[0].flags = 0x4;
	desc[0].limit_low = 0x67;
	desc[0].limit_high = 0;
	desc[0].base_low = (uintptr_t)tss & 0xFFFF;
	desc[0].base_mid = ((uintptr_t)tss >> 16) & 0xFF;
	desc[0].base_high = ((uintptr_t)tss >> 24) & 0xFF;
	desc[1].limit_low = ((uintptr_t)tss >> 32) & 0xFFFF;
	desc[1].base_low = ((uintptr_t)tss >> 48) & 0xFFFF;
}

/**
 * @brief Initializes the Global Descriptor Table (GDT) for the x86_64 architecture.
 *
 * This function sets up the GDT, which is a data structure used by the x86_64
 * architecture to define the characteristics of the various memory segments
 * used in a system. The GDT is essential for memory management and protection.
 */
void gdt_init(void)
{
	_gdt = (gdt *)((uintptr_t)malloc(sizeof(gdt) * numcores + 0x10));
	for (size_t i = 0; i < numcores; i++) {
		write_segment_descriptor(&_gdt[i].entries[0], 0, 0, 0, 0);
		write_segment_descriptor(&_gdt[i].entries[1], 0, 0xFFFFF, 0x9A,
					 0xA);
		write_segment_descriptor(&_gdt[i].entries[2], 0, 0xFFFFF, 0x92,
					 0xC);
		write_segment_descriptor(&_gdt[i].entries[3], 0, 0xFFFFF, 0xFA,
					 0xC);
		write_segment_descriptor(&_gdt[i].entries[4], 0, 0xFFFFF, 0xF2,
					 0xC);
		write_segment_descriptor(&_gdt[i].entries[5], 0, 0xFFFFF, 0xFA,
					 0xA);
		write_tss_segment_descriptor(&_gdt[i].entries[6], &_gdt[i].tss);
		_gdt[i].gdtr.base = (uintptr_t)&_gdt[i].entries;
		_gdt[i].gdtr.limit = sizeof(segment_descriptor) * 8 - 1;
		_gdt[i].tss.iopb_offset = sizeof(tss) & 0xFFFF;
	}

	load_gdt(0);
}
