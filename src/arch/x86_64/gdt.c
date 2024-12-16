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

__attribute__((aligned(0x10))) static segment_descriptor gdt[6] = {
	{ 0, 0, 0, 0, 0, 0, 0 }, 			 // Null descriptor
	{ 0xFFFF, 0, 0, 0x9A, 0xF, 0xA, 0 }, // Code segment
	{ 0xFFFF, 0, 0, 0x92, 0xF, 0xC, 0 }, // Data segment
	{ 0xFFFF, 0, 0, 0xFA, 0xF, 0xA, 0 }, // User mode code segment
	{ 0xFFFF, 0, 0, 0xF2, 0xF, 0xC, 0 }, // User mode data segment
};
static gdtr _gdtr = { .limit = (uint16_t)sizeof(gdt) - 1,
		      .base = (uintptr_t)gdt };

/**
 * @brief Initializes the Global Descriptor Table (GDT) for the x86_64 architecture.
 *
 * This function sets up the GDT, which is a data structure used by the x86_64
 * architecture to define the characteristics of the various memory segments
 * used in a system. The GDT is essential for memory management and protection.
 */
void gdt_init(void)
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
		     :
		     : "m"(_gdtr));
}
