/**
 * @file isrs.S
 * @brief Interrupt Service Routines
 *
 * This file contains the interrupt service routines for the x86_64 architecture.
 * These routines act as wrappers around the C interrupt handlers.
 */

#include "util.S"

_isr_handler:
    cmpq $8, 24(%rsp)
    je 1f
    swapgs
1:  cld
    pusha64
    movq %rsp, %rdi
    call isr_handler
    popa64
    cmpq $8, 24(%rsp)
    je 1f
    swapgs
1:  addq $16, %rsp
    iretq

isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31

isr_no_err_stub 48

.global isr_stub_table
isr_stub_table:
    isr_name 0
    isr_name 1
    isr_name 2
    isr_name 3
    isr_name 4
    isr_name 5
    isr_name 6
    isr_name 7
    isr_name 8
    isr_name 9
    isr_name 10
    isr_name 11
    isr_name 12
    isr_name 13
    isr_name 14
    isr_name 15
    isr_name 16
    isr_name 17
    isr_name 18
    isr_name 19
    isr_name 20
    isr_name 21
    isr_name 22
    isr_name 23
    isr_name 24
    isr_name 25
    isr_name 26
    isr_name 27
    isr_name 28
    isr_name 29
    isr_name 30
    isr_name 31

    isr_name 48
