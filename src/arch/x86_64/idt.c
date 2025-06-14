/**
 * @file idt.c
 * @brief Interrupt Descriptor Table implementation for x86_64 architecture.
 *
 * This file contains the implementation of the Interrupt Descriptor Table for the x86_64 architecture.
 * It allows for the initialization of the IDT and the handling of interrupts and exceptions.
 */

#include <debug.h>

#include <arch/x86_64/apic.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/paging.h>
#include <arch/x86_64/msr.h>
#include <memory.h>
#include <task.h>

__attribute__((aligned(0x10))) static interrupt_descriptor idt[256];
static idtr _idtr = { .limit = (uint16_t)sizeof(idt) - 1,
		      .base = (uintptr_t)idt };

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

/**
 * @brief Handles a panic.
 *
 * This function is called when a panic occurs. It prints the state of the CPU registers
 * at the time of the panic and halts the system.
 *
 * @param frame A pointer to the interrupt frame containing the state
 *              of the CPU registers at the time of the panic.
 */
[[noreturn]] void panic_handler(struct interrupt_frame *frame)
{
	uint64_t cr2;
	asm volatile("movq %%cr2, %0" : "=r"(cr2));
	DEBUG_PRINTF("=== PANIC! ===\n"
		     " - Unhandled %s (%d) @ %#016lX!\n",
		     exception_names[frame->isr_number], frame->error_code,
		     frame->rip);
	DEBUG_PRINTF("RSP: %#016lX\n", frame->rsp);
	DEBUG_PRINTF("RFLAGS: %#016lX\n", frame->rflags);
	DEBUG_PRINTF("CS:SS: %#08lX:%#08lX\n", frame->cs, frame->ss);
	for (int i = 0; i < 15; ++i)
		DEBUG_PRINTF("%3s : %016lX\n", register_names[i],
			     ((uint64_t *)frame)[14 - i]);
	if (frame->isr_number == 0xE)
		DEBUG_PRINTF("Fault address: %#016lX\n", cr2);
	for (;;)
		asm volatile("hlt");
}

static void handle_page_fault(struct interrupt_frame *frame)
{
	uint64_t addr;
	asm volatile("movq %%cr2, %0" : "=r"(addr));
	thread_info *info = (thread_info *)read_msr(MSR_GS_BASE);
	if (!info->thread) {
		panic_handler(frame);
		return;
	}
	uint64_t *pml4 = info->thread->proc->tables;
	uint64_t pml4_i = PML4_INDEX(addr);
	uint64_t pdpt_i = PDPT_INDEX(addr);
	uint64_t pd_i = PD_INDEX(addr);
	uint64_t pt_i = PT_INDEX(addr);

	if (!(pml4[pml4_i] & P_X64_PRESENT)) {
		panic_handler(frame);
		return;
	}
	uint64_t *pdpt = (uint64_t *)(pml4[pml4_i] & ~0xFFF);
	if (!(pdpt[pdpt_i] & P_X64_PRESENT)) {
		panic_handler(frame);
		return;
	}
	uint64_t *pd = (uint64_t *)(pdpt[pdpt_i] & ~0xFFF);
	if (!(pd[pd_i] & P_X64_PRESENT)) {
		panic_handler(frame);
		return;
	}
	uint64_t *pt = (uint64_t *)(pd[pd_i] & ~0xFFF);
	if (!(pt[pt_i] & P_X64_PRESENT) || !(pt[pt_i] & P_X64_COW)) {
		panic_handler(frame);
		return;
	}

	uintptr_t old_paddr = pt[pt_i] & ~0xFFF;
	uintptr_t new_paddr = find_free_frames(1);
	if (new_paddr == (uintptr_t)-1) {
		panic_handler(frame);
		return;
	}
	set_frame_lock(new_paddr, 1, true);
	uint8_t buffer[PAGE_SIZE];
	memcpy(buffer, (void *)addr, PAGE_SIZE);
	paging_map_page(pml4, addr, new_paddr, P_USER_WRITE);
	memcpy((void *)addr, buffer, PAGE_SIZE);
	frame_ref_dec(old_paddr);
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
void isr_handler(struct interrupt_frame *frame)
{
	if (frame->isr_number < 32) {
		if (frame->isr_number == 14)
			handle_page_fault(frame);
		else
			panic_handler(frame);
	} else if (frame->isr_number == 48)
		timer_tick(frame);
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
	descriptor->ist = 1; // Use IST1 for interrupts
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
