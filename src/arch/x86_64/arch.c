#include <arch.h>

void gdt_init(void);
void idt_init(void);

void arch_init(void)
{
	gdt_init();
	idt_init();
}
