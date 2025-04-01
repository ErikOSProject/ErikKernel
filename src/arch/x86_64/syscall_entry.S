/**
 * @file syscall_entry.S
 * @brief System Call Entry
 *
 * This file contains the system call entry point for the x86_64 architecture.
 */

#include "util.S"

.global syscall_entry
syscall_entry:
    swapgs
    movq %rsp, %gs:0x10 // save user stack pointer
    movq %gs:0x8, %rsp  // switch to kernel stack

    // Push fake interrupt stack
    pushq $0x23
    pushq %gs:0x10
    pushq %r11
    pushq $0x2B
    pushq %rcx
    pushq $0
    pushq $0

    pusha64
    movq %rsp, %rdi
    call syscall_handler
    popa64

    swapgs
    addq $16, %rsp
    iretq

.global task_idle
task_idle:
    hlt
    jmp task_idle
