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

void isr_handler(interrupt_frame *frame)
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
	asm volatile("1: cli; hlt; jmp 1b");
}

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

void idt_init(void)
{
	for (uint8_t vector = 0; vector < 32; vector++)
		idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);

	asm volatile("lidt %0; sti" : : "m"(_idtr));
}
