/**
 * @file main.c
 * @brief Main entry point for the ErikKernel project.
 */

#include <acpi.h>
#include <arch.h>
#include <debug.h>
#include <erikboot.h>
#include <fs.h>
#include <heap.h>
#include <memory.h>
#include <paging.h>

/**
 * @brief Entry point for the kernel.
 * 
 * This function is marked with [[noreturn]] indicating that it will not return to the caller.
 * It serves as the main entry point for the kernel and is responsible for initializing
 * the system using the provided boot information.
 * 
 * @param boot_info A structure containing information passed by the bootloader.
 */
[[noreturn]] void kernel_main(BootInfo boot_info)
{
	DEBUG_INIT();
	DEBUG_PRINTF("Hello world from ErikKernel!\n\n");

	arch_init(&boot_info);
	page_frame_allocator_init(&boot_info);
	heap_init(&boot_info);
	fs_init(&boot_info);
	smp_init(&boot_info);
	DEBUG_PRINTF("OK!\n");
	for (;;)
		asm volatile("hlt");
}
