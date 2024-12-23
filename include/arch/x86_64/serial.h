/**
 * @file serial.h
 * @brief Serial port driver for x86_64 architecture.
 *
 * This file contains the declarations for the serial port driver for the x86_64 architecture.
 * It allows for the initialization of the serial port and the writing of characters to it.
 */

#ifndef _ARCH_SERIAL_H
#define _ARCH_SERIAL_H

#include <serial.h>

#define COM1 0x3f8
#define UART_FREQ 115200

typedef struct {
	uint64_t base_port;
	uint32_t baudrate;
	uint32_t data_bits;
	uint32_t stop_bits;
} x86_serial;

#endif //_ARCH_SERIAL_H
