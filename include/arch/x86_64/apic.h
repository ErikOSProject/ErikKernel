/**
 * @file apic.h
 * @brief Local APIC implementation for x86_64 architecture.
 *
 * This file contains the declarations for the Local APIC implementation for the x86_64 architecture.
 * It allows for the initialization of the Local APIC and the setting of interrupt handlers.
 */

#ifndef _APIC_H
#define _APIC_H

#include <erikboot.h>
#include <arch/x86_64/idt.h>

typedef struct {
	uint32_t *address;
	uint32_t base;
	uint8_t length;
} IOAPIC;

#define APIC_EOI 0x2C
#define APIC_ERROR 0xA0
#define APIC_ICR_LOW 0xC0
#define APIC_ICR_HIGH 0xC4
#define APIC_LVT_TIMER 0xC8
#define APIC_TIMER_INITCNT 0xE0
#define APIC_DIV_TIMER 0xFA

extern uint8_t numcores;

extern uintptr_t ap_entry;
extern uintptr_t ap_callback;
extern uintptr_t ap_stacks;

void set_core_base(uint64_t id);
void apic_init(BootInfo *boot_info);
void timer_tick(struct interrupt_frame *frame);

#endif //_APIC_H
