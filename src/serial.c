/**
 * @file serial.c
 * @brief Implementation of serial communication functions.
 *
 * This file contains the implementation of functions for serial communication.
 * It includes the necessary headers and provides the functionality required
 * for initializing and handling serial ports.
 */

#include <serial.h>

serial_device primary_serial = { NULL, NULL };

/**
 * @brief Initializes the serial communication interface.
 *
 * This function sets up the necessary configurations to enable
 * serial communication. It should be called before any serial
 * communication functions are used.
 */
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

/**
 * @brief Sends a character to the serial port.
 *
 * This function transmits a single character to the serial port, which can be
 * used for debugging or communication purposes.
 *
 * @param c The character to be sent to the serial port.
 */
void serial_putchar(char c)
{
	if (!primary_serial.driver || !primary_serial.driver->send)
		return;
	primary_serial.driver->send(primary_serial.data, c);
}

/**
 * @brief Prints a string to the serial port.
 *
 * This function sends each character of the given string to the serial port
 * for output. It is typically used for debugging purposes to output text
 * to a serial console.
 *
 * @param str The null-terminated string to be printed.
 */
void serial_print(const char *str)
{
	if (!primary_serial.driver || !primary_serial.driver->send)
		return;
	for (; *str; str++)
		primary_serial.driver->send(primary_serial.data, *str);
}
