/**
 * @file serial.c
 * @brief Serial communication implementation for x86_64 architecture.
 *
 * This file contains the implementation of serial communication functions
 * specific to the x86_64 architecture. It includes necessary headers and
 * provides the functionality required for serial input/output operations.
 */

#include <serial.h>

#define COM1 0x3f8
#define UART_FREQ 115200

/**
 * @brief Writes a byte to the specified port.
 *
 * This function sends the given 8-bit value to the I/O port specified by the
 * port address. It is typically used for low-level hardware communication.
 *
 * @param port The I/O port address to write to.
 * @param val The 8-bit value to write to the port.
 */
static inline void outb(uint16_t port, uint8_t val)
{
	asm volatile("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

/**
 * @brief Reads a byte from the specified I/O port.
 *
 * @param port The I/O port to read from.
 * @return The byte read from the I/O port.
 */
static inline uint8_t inb(uint16_t port)
{
	uint8_t ret;
	asm volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
	return ret;
}

typedef struct {
	uint64_t base_port;
	uint32_t baudrate;
	uint32_t data_bits;
	uint32_t stop_bits;
} x86_serial;

x86_serial _x86_serial_default = { COM1, 115200, 8, 1 };

/**
 * @brief Calculates the divisors for the serial device.
 *
 * This function computes the high and low divisors required for the
 * specified serial device to achieve the desired baud rate.
 *
 * @param dev Pointer to the x86_serial structure representing the serial device.
 * @param hi Pointer to a uint8_t variable where the high divisor will be stored.
 * @param lo Pointer to a uint8_t variable where the low divisor will be stored.
 */
static void x86_serial_calculate_divisors(const x86_serial *dev, uint8_t *hi,
					  uint8_t *lo)
{
	const uint16_t div = UART_FREQ / dev->baudrate;

	*lo = div & 0xff;
	*hi = (div >> 8) & 0xff;
}

/**
 * @brief Waits for the transmission to complete on the specified serial device.
 *
 * This function blocks until the transmission is complete on the given
 * x86_serial device. It ensures that all data has been transmitted before
 * proceeding.
 *
 * @param dev Pointer to the x86_serial device structure.
 */
static void x86_serial_wait_tx_complete(const x86_serial *dev)
{
	while ((inb(dev->base_port + 5) & 0x20) == 0)
		;
}

/**
 * @brief Resets the specified x86 serial device.
 *
 * This function performs a reset operation on the given x86 serial device.
 *
 * @param dev A pointer to the x86_serial structure representing the serial device to be reset.
 * @return An integer indicating the success or failure of the reset operation.
 *         Zero indicates success, while a non-zero value indicates an error.
 */
int x86_serial_reset(x86_serial *dev)
{
	uint8_t div_hi, div_lo;

	x86_serial_calculate_divisors(dev, &div_hi, &div_lo);
	uint8_t line_ctrl = (dev->data_bits - 5) | ((dev->stop_bits - 1) << 2);

	outb(dev->base_port + 1, 0x00); // Disable all interrupts
	outb(dev->base_port + 3, 0x80); // Enable DLAB (set baud rate divisor)
	outb(dev->base_port + 0, div_lo); // Set divisor lo byte
	outb(dev->base_port + 1, div_hi); //             hi byte
	outb(dev->base_port + 3,
	     line_ctrl); // Set line control (data bits, parity, and stop bits)
	outb(dev->base_port + 2,
	     0xC7); // Enable FIFO, clear them, with 14-byte threshold
	outb(dev->base_port + 4,
	     0x12); // Set in loopback mode, test the serial chip
	outb(dev->base_port + 0, 0xAE); // Test serial chip

	// Check if serial is faulty
	if (inb(dev->base_port + 0) != 0xAE)
		return 1;

	// If serial is not faulty set it in normal operation mode
	outb(dev->base_port + 4, 0x0F);
	return 0;
}

/**
 * @brief Sets up the x86 serial device.
 *
 * This function initializes and configures the specified x86 serial device.
 *
 * @param dev A pointer to the x86_serial structure representing the serial device to be set up.
 * @return An integer indicating the success or failure of the setup operation.
 *         Zero indicates success, while a non-zero value indicates an error.
 */
int x86_serial_setup(x86_serial *dev)
{
	return x86_serial_reset(dev);
}

/**
 * @brief Sends a character to the specified serial device.
 *
 * This function writes a single character to the serial device specified
 * by the `dev` parameter. It is typically used for low-level debugging
 * or communication purposes.
 *
 * @param dev Pointer to the x86_serial device structure.
 * @param c The character to be sent to the serial device.
 */
void x86_serial_putchar(x86_serial *dev, char c)
{
	x86_serial_wait_tx_complete(dev);
	outb(dev->base_port, c);
}

serial_driver x86_serial_driver = {
	(int (*)(void *))x86_serial_setup, (int (*)(void *))x86_serial_reset,
	(void (*)(void *, char))x86_serial_putchar
};
