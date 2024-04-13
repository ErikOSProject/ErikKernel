#include <erikboot.h>
#include <serial.h>

[[noreturn]] void kernel_main(BootInfo boot_info)
{
	serial_init();
	serial_print("Hello world from ErikKernel!\r\n");
	if (boot_info.FBBase)
		for (uint32_t *p = (uint32_t *)boot_info.FBBase;
		     p <
		     (uint32_t *)boot_info.FBBase +
			     boot_info.FBPixelsPerScanLine * boot_info.FBHeight;
		     p++) {
			*p = 0xff00ff00;
		}
	for (;;)
		;
}
