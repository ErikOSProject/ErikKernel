/**
 * @file syscall.h
 * @brief System call interface for x86_64 architecture.
 *
 * This file contains the declarations for the system call interface for the x86_64 architecture.
 * It allows for the invocation of system calls from user space.
 */

#ifndef _ARCH_SYSCALL_H
#define _ARCH_SYSCALL_H

typedef struct {
	uint64_t cpuid;
	uint64_t kernel_stack;
	uint64_t user_stack;
} __attribute__((packed)) thread_info;

void syscall_init(void);
void syscall_entry(void);

#endif //_ARCH_SYSCALL_H
