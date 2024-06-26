#include <arch.h>
#include <debug.h>
#include <erikboot.h>
#include <fs.h>
#include <heap.h>
#include <memory.h>
#include <paging.h>

[[noreturn]] void kernel_main(BootInfo boot_info)
{
	DEBUG_INIT();
	DEBUG_PRINTF("Hello world from ErikKernel!\n\n");

	arch_init();
	page_frame_allocator_init(&boot_info);
	heap_init(&boot_info);
	fs_init(&boot_info);
	DEBUG_PRINTF("OK!\n");

	for (;;)
		(void)boot_info;
}
