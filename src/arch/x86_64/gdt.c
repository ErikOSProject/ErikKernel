/**
 * @file gdt.c
 * @brief Global Descriptor Table implementation for x86_64 architecture.
 *
 * This file contains the implementation of the Global Descriptor Table for the x86_64 architecture.
 * It allows for the initialization of the GDT and the setting of segment descriptors.
 */

#include <debug.h>

typedef struct {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t limit_high : 4;
	uint8_t flags : 4;
	uint8_t base_high;
} __attribute__((packed)) segment_descriptor;

typedef struct {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) gdtr;

__attribute__((aligned(0x10))) static segment_descriptor gdt[6];
static gdtr _gdtr = { .limit = (uint16_t)sizeof(gdt) - 1,
		      .base = (uintptr_t)gdt };

/**
 * @brief Sets a descriptor in the Global Descriptor Table (GDT).
 *
 * This function configures a GDT descriptor with the specified base address,
 * limit, and access flags. The GDT is used in x86_64 architecture to define
 * the characteristics of the various memory segments used by the CPU.
 *
 * @param selector The index of the descriptor in the GDT.
 * @param base The base address of the segment.
 * @param limit The limit of the segment.
 */
void gdt_set_descriptor(uint8_t selector, uint32_t base, uint32_t limit,
			uint8_t access, uint8_t flags)
{
	segment_descriptor *descriptor = &gdt[selector / 8];

	descriptor->base_low = base & 0xFFFF;
	descriptor->base_mid = (base >> 16) & 0xFF;
	descriptor->base_high = (base >> 24) & 0xFF;
	descriptor->limit_low = limit & 0xFFFF;
	descriptor->limit_high = (limit >> 16) & 0xF;
	descriptor->access = access;
	descriptor->flags = flags;
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
	gdt_set_descriptor(0, 0, 0, 0, 0);
	gdt_set_descriptor(0x8, 0, 0xFFFFF, 0x9A, 0xA);
	gdt_set_descriptor(0x10, 0, 0xFFFFF, 0x92, 0xC);
	gdt_set_descriptor(0x18, 0, 0xFFFFF, 0xFA, 0xA);
	gdt_set_descriptor(0x20, 0, 0xFFFFF, 0xF2, 0xC);
	gdt_set_descriptor(0x28, 0, 0, 0, 0);

	asm volatile("lgdt %0;"
		     "pushq $0x8;"
		     "leaq 1f, %%rax;"
		     "pushq %%rax;"
		     "lretq;"
		     "movw $0x10, %%ax;"
		     "movw %%ax, %%ds;"
		     "movw %%ax, %%es;"
		     "movw %%ax, %%fs;"
		     "movw %%ax, %%gs;"
		     "movw %%ax, %%ss;"
		     "1:"
		     :
		     : "m"(_gdtr));
}
