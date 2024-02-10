#include <erikboot.h>

[[noreturn]] void kernel_main(BootInfo boot_info)
{
	for (uint32_t *p = (uint32_t *)boot_info.FBBase;
	     p < (uint32_t *)boot_info.FBBase +
			 boot_info.FBPixelsPerScanLine * boot_info.FBHeight;
	     p++) {
		*p = 0xffff00ff;
	}
	for (;;)
		;
}
