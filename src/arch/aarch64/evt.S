/**
 * @file evt.S
 * @brief Exception vector table for AArch64
 *
 * This file contains the exception vector table for the AArch64 architecture.
 * The contained routines act as wrappers around the C exception handlers.
 */

save_corruptible:
stp x0, x1, [sp, #-16]!
stp x2, x3, [sp, #-16]!
stp x4, x5, [sp, #-16]!
stp x6, x7, [sp, #-16]!
stp x8, x9, [sp, #-16]!
stp x10, x11, [sp, #-16]!
stp x12, x13, [sp, #-16]!
stp x14, x15, [sp, #-16]!
ret

restore_corruptible:
ldp x14, x15, [sp], #16
ldp x12, x13, [sp], #16
ldp x10, x11, [sp], #16
ldp x8, x9, [sp], #16
ldp x6, x7, [sp], #16
ldp x4, x5, [sp], #16
ldp x2, x3, [sp], #16
ldp x0, x1, [sp], #16
ret

.balign 0x800
.global vector_table_el1
vector_table_el1:
curr_el_sp0_sync:
bl save_corruptible
bl handle_synchronous_exception
bl restore_corruptible
eret

.balign 0x80
curr_el_sp0_irq:
nop

.balign 0x80
curr_el_sp0_fiq:
nop

.balign 0x80
curr_el_sp0_serror:
nop

.balign 0x80
curr_el_spx_sync:
bl save_corruptible
mov x0, sp
bl handle_synchronous_exception
bl restore_corruptible
eret

.balign 0x80
curr_el_spx_irq:
nop

.balign 0x80
curr_el_spx_fiq:
nop

.balign 0x80
curr_el_spx_serror:
nop

.balign 0x80
lower_el_64_sync:
nop

.balign 0x80
lower_el_64_irq:
nop

.balign 0x80
lower_el_64_fiq:
nop

.balign 0x80
lower_el_64_serror:
nop

.balign 0x80
lower_el_32_sync:
nop

.balign 0x80
lower_el_32_irq:
nop

.balign 0x80
lower_el_32_fiq:
nop

.balign 0x80
lower_el_32_serror:
nop
