/**
 * @file io.h
 * @brief Input/Output operations for x86_64 architecture.
 *
 * This file contains the declarations for the input/output operations for the x86_64 architecture.
 * It allows for the reading and writing of data to and from I/O ports.
 */

#ifndef _ARCH_IO_H
#define _ARCH_IO_H

#include <stdint.h>

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

#endif //_ARCH_IO_H
