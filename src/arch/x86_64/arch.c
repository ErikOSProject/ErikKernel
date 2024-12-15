/**
 * @file arch.c
 * @brief Architecture-specific initialization code.
 *
 * This file contains the implementation of the architecture-specific initialization code for x86_64 architecture.
 */

#include <arch.h>

void gdt_init(void);
void idt_init(void);
void get_pml4(void);
void get_lapic(BootInfo *boot_info);

/**
 * @brief Initializes the architecture-specific components.
 *
 * This function is responsible for setting up and initializing
 * all architecture-specific components and configurations required
 * for the system to operate correctly on x86_64 architecture.
 */
void arch_init(BootInfo *boot_info)
{
	gdt_init();
	idt_init();
	get_pml4();
	get_lapic(boot_info);
}
