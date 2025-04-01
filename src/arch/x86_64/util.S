/**
 * @file util.S
 * @brief Assembly utility macros for x86_64 architecture.
 *
 * This file contains assembly macros for saving and restoring registers
 * when handling interrupts and syscalls, as well as defining interrupt service
 * routine (ISR) stubs.
 */

.macro pusha64
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rbp
    push %rdi
    push %rsi
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
.endm

.macro popa64
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rsi
    pop %rdi
    pop %rbp
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax
.endm

.macro isr_err_stub no
isr_stub_\no:
    pushq $\no
    jmp _isr_handler
.endm

.macro isr_no_err_stub no
isr_stub_\no:
    pushq $0
    pushq $\no
    jmp _isr_handler
.endm

.macro isr_name no
.quad isr_stub_\no
.endm
