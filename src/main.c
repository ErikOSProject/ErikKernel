#include <arch.h>
#include <debug.h>
#include <erikboot.h>
#include <memory.h>

[[noreturn]] void kernel_main(BootInfo boot_info)
{
	DEBUG_INIT();
	DEBUG_PRINTF("Hello world from ErikKernel!\n\n");

	arch_init();
	page_frame_allocator_init(&boot_info);
	DEBUG_PRINTF("OK!\n");

	for (;;)
		(void)boot_info;
}
