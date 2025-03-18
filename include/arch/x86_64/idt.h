/**
 * @file idt.h
 * @brief Interrupt Descriptor Table implementation for x86_64 architecture.
 *
 * This file contains the declarations for the Interrupt Descriptor Table for the x86_64 architecture.
 * It allows for the initialization of the IDT and the setting of interrupt handlers.
 */

#ifndef _IDT_H
#define _IDT_H

typedef struct {
	uint16_t isr_low;
	uint16_t selector;
	uint8_t ist;
	uint8_t attributes;
	uint16_t isr_mid;
	uint32_t isr_high;
	uint32_t reserved;
} __attribute__((packed)) interrupt_descriptor;

typedef struct {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) idtr;

struct interrupt_frame {
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t rbp;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;

	uint64_t isr_number;
	uint64_t error_code;

	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
};

extern void *isr_stub_table[];

void idt_init(void);
void load_idt(void);

#endif //_IDT_H
