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
#include <arch/x86_64/syscall.h>

/**
 * @brief Initializes the architecture-specific components required in early initialization.
 *
 * This function is responsible for setting up and initializing all architecture-specific
 * components and configurations required during early initialization on x86_64 architecture.
 *
 * @param boot_info The boot information structure passed by the bootloader.
 */
void arch_preinit(BootInfo *)
{
	get_pml4();
}

/**
 * @brief Initializes the architecture-specific components.
 *
 * This function is responsible for setting up and initializing
 * all architecture-specific components and configurations required
 * for the system to operate correctly on x86_64 architecture.
 *
 * @param boot_info The boot information structure passed by the bootloader.
 */
void arch_init(BootInfo *boot_info)
{
	apic_init(boot_info);
	gdt_init();
	idt_init();
	syscall_init();
}
