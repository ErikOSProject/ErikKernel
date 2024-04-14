#include <serial.h>

serial_device primary_serial = { NULL, NULL };

void serial_init(void)
{
#ifdef AARCH64_QEMU_UART
	extern serial_driver pl011_driver;
	extern char _pl011_default;
	primary_serial.driver = &pl011_driver;
	primary_serial.data = &_pl011_default;
#endif //AARCH64_QEMU_UART

#ifdef X64_UART
	extern serial_driver x86_serial_driver;
	extern char _x86_serial_default;
	primary_serial.driver = &x86_serial_driver;
	primary_serial.data = &_x86_serial_default;
#endif //X64_UART

	if (primary_serial.driver)
		primary_serial.driver->init(primary_serial.data);
}

void serial_putchar(char c)
{
	if (!primary_serial.driver || !primary_serial.driver->send)
		return;
	primary_serial.driver->send(primary_serial.data, c);
}

void serial_print(const char *str)
{
	if (!primary_serial.driver || !primary_serial.driver->send)
		return;
	for (; *str; str++)
		primary_serial.driver->send(primary_serial.data, *str);
}
