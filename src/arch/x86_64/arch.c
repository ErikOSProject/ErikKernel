/**
 * @file arch.c
 * @brief Architecture-specific initialization code.
 *
 * This file contains the implementation of the architecture-specific initialization code for x86_64 architecture.
 */

#include <arch.h>

#include <arch/x86_64/apic.h>
#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/paging.h>

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
	apic_init(boot_info);
}
