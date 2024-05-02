#include <arch.h>
#include <debug.h>

extern char vector_table_el1;
void get_ttbr0(void);
void get_ttbr1(void);

char *exception_names[] = { "unknown",
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    "data abort in kernel space",
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    0,
			    "64-bit breakpoint",
			    0,
			    0,
			    0 };

void arch_init(void)
{
	asm volatile("msr vbar_el1, %0;"
		     "isb;" ::"r"(&vector_table_el1));

	get_ttbr0();
	get_ttbr1();
}

void handle_synchronous_exception(uint64_t *frame)
{
	(void)frame;
	uint64_t esr, elr, far;
	asm volatile("mrs %0, esr_el1;" : "=r"(esr));
	asm volatile("mrs %0, elr_el1;" : "=r"(elr));
	asm volatile("mrs %0, far_el1;" : "=r"(far));
	uint8_t ec = (esr >> 26) & 0x3F;
	DEBUG_PRINTF("=== PANIC! ===\n"
		     " - Unhandled %s @ %#016lX!\n",
		     exception_names[ec], elr);
	for (int i = 0; i < 15; ++i)
		DEBUG_PRINTF("X%-2i : %016lX\n", i, frame[14 - i]);
	if (ec == 0x25)
		DEBUG_PRINTF("Fault address: %#016lX\n", far);
	asm volatile("1: wfi; b 1b");
}
