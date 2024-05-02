#include <arch.h>

void gdt_init(void);
void idt_init(void);
void get_pml4(void);

void arch_init(void)
{
	gdt_init();
	idt_init();
	get_pml4();
}
