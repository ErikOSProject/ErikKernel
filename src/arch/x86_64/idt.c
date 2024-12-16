/**
 * @file idt.c
 * @brief Interrupt Descriptor Table implementation for x86_64 architecture.
 *
 * This file contains the implementation of the Interrupt Descriptor Table for the x86_64 architecture.
 * It allows for the initialization of the IDT and the handling of interrupts and exceptions.
 */

#include <debug.h>

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

__attribute__((aligned(0x10))) static interrupt_descriptor idt[256];
static idtr _idtr = { .limit = (uint16_t)sizeof(idt) - 1,
		      .base = (uintptr_t)idt };

typedef struct {
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
} interrupt_frame;

char *exception_names[] = { "division by zero",
			    "debug exception",
			    "non-maskable interrupt",
			    "breakpoint",
			    "overflow",
			    "bound range exeeded",
			    "invalid opcode",
			    "device unavailable exception",
			    "double fault",
			    "coprocessor segment overrun",
			    "invalid TSS exception",
			    "segmentation fault",
			    "stack-segment fault",
			    "general protection fault",
			    "page fault",
			    "reserved exception (this should not happen)",
			    "FPU error",
			    "alignment check error",
			    "machine check error",
			    "SIMD exception",
			    "virtualization exception",
			    "control protection exception",
			    "reserved exception (this should not happen)",
			    "reserved exception (this should not happen)",
			    "reserved exception (this should not happen)",
			    "reserved exception (this should not happen)",
			    "reserved exception (this should not happen)",
			    "reserved exception (this should not happen)",
			    "reserved exception (this should not happen)",
			    "reserved exception (this should not happen)",
			    "reserved exception (this should not happen)",
			    "reserved exception (this should not happen)" };

char *register_names[] = {
	"RAX", "RBX", "RCX", "RDX", "RBP", "RDI", "RSI", "R8",
	"R9",  "R10", "R11", "R12", "R13", "R14", "R15",
};

extern void *isr_stub_table[];

void timer_tick(void);
void timer_propagate(void);

/**
 * @brief Handles a panic.
 *
 * This function is called when a panic occurs. It prints the state of the CPU registers
 * at the time of the panic and halts the system.
 *
 * @param frame A pointer to the interrupt frame containing the state
 *              of the CPU registers at the time of the panic.
 */
[[noreturn]] void panic_handler(interrupt_frame *frame)
{
	uint64_t cr2;
	asm volatile("movq %%cr2, %0" : "=r"(cr2));
	DEBUG_PRINTF("=== PANIC! ===\n"
		     " - Unhandled %s @ %#016lX!\n",
		     exception_names[frame->isr_number], frame->rip);
	for (int i = 0; i < 15; ++i)
		DEBUG_PRINTF("%3s : %016lX\n", register_names[i],
			     ((uint64_t *)frame)[14 - i]);
	if (frame->isr_number == 0xE)
		DEBUG_PRINTF("Fault address: %#016lX\n", cr2);
	for (;;)
		asm volatile("hlt");
}

/**
 * @brief Handles the interrupt service routine (ISR).
 *
 * This function is called when an interrupt occurs. It processes the
 * interrupt and performs the necessary actions to handle it.
 *
 * @param frame A pointer to the interrupt frame containing the state
 *              of the CPU registers at the time of the interrupt.
 */
interrupt_frame *isr_handler(interrupt_frame *frame)
{
	if (frame->isr_number < 32)
		panic_handler(frame);

	else if (frame->isr_number == 48)
		timer_tick();

	return frame;
}

/**
 * @brief Sets an Interrupt Descriptor Table (IDT) entry.
 *
 * This function sets an entry in the IDT with the specified vector, 
 * interrupt service routine (ISR) address, and flags.
 *
 * @param vector The interrupt vector number.
 * @param isr Pointer to the interrupt service routine.
 * @param flags Flags for the IDT entry.
 */
void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags)
{
	interrupt_descriptor *descriptor = &idt[vector];

	descriptor->isr_low = (uint64_t)isr & 0xFFFF;
	descriptor->selector = 0x8;
	descriptor->ist = 0;
	descriptor->attributes = flags;
	descriptor->isr_mid = ((uint64_t)isr >> 16) & 0xFFFF;
	descriptor->isr_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
	descriptor->reserved = 0;
}

/**
 * @brief Loads the Interrupt Descriptor Table (IDT).
 *
 * This function loads the IDT by writing the address of the IDT to the
 * IDTR register and enabling interrupts.
 */
void load_idt(void)
{
	asm volatile("lidt %0; sti" : : "m"(_idtr));
}

/**
 * @brief Initializes the Interrupt Descriptor Table (IDT).
 *
 * This function sets up the IDT for the x86_64 architecture, which is
 * essential for handling interrupts and exceptions. It configures the
 * necessary entries in the IDT to ensure proper interrupt handling.
 */
void idt_init(void)
{
	uint8_t vector = 0;
	for (; vector < 0x20; vector++)
		idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
	for (; vector < 0x21; vector++)
		idt_set_descriptor(0x10 + vector, isr_stub_table[vector], 0x8E);

	load_idt();
}
