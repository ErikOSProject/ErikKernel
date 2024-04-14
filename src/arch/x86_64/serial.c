#include <serial.h>

#define COM1 0x3f8
#define UART_FREQ 115200

static inline void outb(uint16_t port, uint8_t val)
{
	asm volatile("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

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

static void x86_serial_calculate_divisors(const x86_serial *dev, uint8_t *hi,
					  uint8_t *lo)
{
	const uint16_t div = UART_FREQ / dev->baudrate;

	*lo = div & 0xff;
	*hi = (div >> 8) & 0xff;
}

static void x86_serial_wait_tx_complete(const x86_serial *dev)
{
	while ((inb(dev->base_port + 5) & 0x20) == 0)
		;
}

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

int x86_serial_setup(x86_serial *dev)
{
	return x86_serial_reset(dev);
}

void x86_serial_putchar(x86_serial *dev, char c)
{
	x86_serial_wait_tx_complete(dev);
	outb(dev->base_port, c);
}

serial_driver x86_serial_driver = {
	(int (*)(void *))x86_serial_setup, (int (*)(void *))x86_serial_reset,
	(void (*)(void *, char))x86_serial_putchar
};
