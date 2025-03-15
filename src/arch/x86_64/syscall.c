/**
 * @file syscall.c
 * @brief System call implementation for x86_64 architecture.
 *
 * This file contains the implementation of the system call interface for the x86_64 architecture.
 * It allows for the invocation of system calls from user space.
 */

#include <debug.h>

#include <arch/x86_64/msr.h>
#include <arch/x86_64/syscall.h>

void user_test(void);

/**
 * @brief System call entry point.
 *
 * This function is the C entry point for system calls. It is called by the assembly
 * stub that is invoked by the SYSCALL instruction.
 */
void syscall_handler(void)
{
	DEBUG_PRINTF("System call!\n");
}

/**
 * @brief Enter user space.
 *
 * This function is used to enter user space. It is called by the kernel_main function
 * after initializing the system.
 */
void enter_user(void)
{
	asm volatile(
		"movq %0, %%rcx; movq $0x202, %%r11; sysretq" ::"i"(&user_test)
		: "rcx", "r11");
}

/**
 * @brief Initialize the system call interface.
 *
 * This function initializes the system call interface by enabling the syscall/sysret
 * instructions and setting the appropriate model-specific registers (MSRs).
 */
void syscall_init(void)
{
	// Enable syscall/sysret instructions
	uint64_t efer = read_msr(MSR_EFER);
	efer |= EFER_SCE;
	write_msr(MSR_EFER, efer);

	// Set syscall/sysret CS and SS selectors
	write_msr(MSR_STAR, ((uint64_t)0x8 << 32) | ((uint64_t)0x18 << 48));
	write_msr(MSR_LSTAR, (uint64_t)syscall_entry);
	write_msr(MSR_SFMASK, 0x700);
}
