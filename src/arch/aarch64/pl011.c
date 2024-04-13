#include <serial.h>

#define DR_OFFSET 0x000
#define FR_OFFSET 0x018
#define IBRD_OFFSET 0x024
#define FBRD_OFFSET 0x028
#define LCR_OFFSET 0x02c
#define CR_OFFSET 0x030
#define IMSC_OFFSET 0x038
#define DMACR_OFFSET 0x048

#define FR_BUSY (1 << 3)

#define CR_TXEN (1 << 8)
#define CR_UARTEN (1 << 0)

#define LCR_FEN (1 << 4)
#define LCR_STP2 (1 << 3)

#define REG(dev, offset) ((uint32_t *)(dev->base_address + offset))

typedef struct {
	uint64_t base_address;
	uint64_t base_clock;
	uint32_t baudrate;
	uint32_t data_bits;
	uint32_t stop_bits;
} pl011;

pl011 _pl011_default = { 0x9000000, 24000000, 115200, 8, 1 };

static void pl011_calculate_divisors(const pl011 *dev, uint32_t *integer,
				     uint32_t *fractional)
{
	const uint32_t div = 4 * dev->base_clock / dev->baudrate;

	*fractional = div & 0x3f;
	*integer = (div >> 6) & 0xffff;
}

static void pl011_wait_tx_complete(const pl011 *dev)
{
	while ((*REG(dev, FR_OFFSET) & FR_BUSY) != 0) {
	}
}

int pl011_reset(pl011 *dev)
{
	uint32_t *cr = REG(dev, CR_OFFSET);
	uint32_t *lcr = REG(dev, LCR_OFFSET);
	uint32_t ibrd, fbrd;

	*cr = (*cr & CR_UARTEN);
	pl011_wait_tx_complete(dev);

	*lcr = (*lcr & ~LCR_FEN);

	pl011_calculate_divisors(dev, &ibrd, &fbrd);
	*REG(dev, IBRD_OFFSET) = ibrd;
	*REG(dev, FBRD_OFFSET) = fbrd;

	*lcr = ((dev->data_bits - 1) & 0x3) << 5;
	if (dev->stop_bits == 2)
		*lcr |= (7 & 0x3) << 5;

	*REG(dev, IMSC_OFFSET) = 0x7ff;
	*REG(dev, DMACR_OFFSET) = 0x0;
	*REG(dev, CR_OFFSET) = CR_TXEN;
	*REG(dev, CR_OFFSET) = CR_TXEN | CR_UARTEN;

	return 0;
}

int pl011_setup(pl011 *dev)
{
	return pl011_reset(dev);
}

void pl011_putchar(pl011 *dev, char c)
{
	pl011_wait_tx_complete(dev);
	*REG(dev, DR_OFFSET) = c;
}

serial_driver pl011_driver = { (int (*)(void *))pl011_setup,
			       (int (*)(void *))pl011_reset,
			       (void (*)(void *, char))pl011_putchar };
