/**
 * @file gdt.h
 * @brief Global Descriptor Table implementation for x86_64 architecture.
 *
 * This file contains the declarations for the Global Descriptor Table for the x86_64 architecture.
 * It allows for the initialization of the GDT and the setting of segment descriptors.
 */

#ifndef _GDT_H
#define _GDT_H

typedef struct {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t limit_high : 4;
	uint8_t flags : 4;
	uint8_t base_high;
} __attribute__((packed)) segment_descriptor;

typedef struct {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) gdtr;

void gdt_init(void);

#endif //_GDT_H
