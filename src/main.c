#include <debug.h>
#include <erikboot.h>

[[noreturn]] void kernel_main(BootInfo boot_info)
{
	DEBUG_INIT();
	DEBUG_PRINTF("Hello world from ErikKernel!\n\n");

	const char *s = "Hello";
	DEBUG_PRINTF("Strings:\n");
	DEBUG_PRINTF(" padding:\n");
	DEBUG_PRINTF("\t[%10s]\n", s);
	DEBUG_PRINTF("\t[%-10s]\n", s);
	DEBUG_PRINTF("\t[%*s]\n", 10, s);
	DEBUG_PRINTF(" truncating:\n");
	DEBUG_PRINTF("\t%.4s\n", s);
	DEBUG_PRINTF("\t%.*s\n", 3, s);

	DEBUG_PRINTF("Characters:\t%c %%\n", 'A');

	DEBUG_PRINTF("Integers:\n");
	DEBUG_PRINTF("\tDecimal:\t%i %d %.6i %i %.0i %+i %i\n", 1, 2, 3, 0, 0,
		     4, -4);
	DEBUG_PRINTF("\tHexadecimal:\t%x %x %X %#x\n", 5, 10, 10, 6);
	DEBUG_PRINTF("\tOctal:\t\t%o %#o %#o\n", 10, 10, 4);

	DEBUG_PRINTF("Floating point:\n");
	DEBUG_PRINTF("\tRounding:\t%f %.0f %.12f %.1f\n", 1.5, 1.5, 1.3, 1.25);
	DEBUG_PRINTF("\tPadding:\t%05.2f %.2f %5.2f\n", 1.5, 1.5, 1.5);
	DEBUG_PRINTF("\tScientific:\t%E %e\n", 150.5, 0.015);
	DEBUG_PRINTF("\tHexadecimal:\t%a %A\n", 100.5, 0.125);

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
