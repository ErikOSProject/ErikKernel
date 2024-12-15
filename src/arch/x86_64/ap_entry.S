.code16
.section .text
.global ap_entry

ap_entry:
    cli                 # Clear interrupts
    cld                 # Clear direction flag
    ljmp $0, $(0x8000 + real_mode_entry - ap_entry)

.align 16
.global ap_callback
ap_callback:
    .quad 0
.global ap_pml4
ap_pml4:
    .quad 0
.global ap_stacks
ap_stacks:
    .quad 0
gdt:
    .long 0, 0
    .long 0x0000FFFF, 0x00CF9A00    # flat code
    .long 0x0000FFFF, 0x008F9200    # flat data
    .long 0x0000FFFF, 0x00AF9A00    # flat long code
gdtr:
    .word gdtr - gdt
    .long 0x8000 + gdt - ap_entry
    .quad 0

.align 64
real_mode_entry:
    # Load the GDT
    xorw    %ax, %ax
    movw    %ax, %ds
    lgdtl   0x8000 + gdtr - ap_entry

    # Enable protected mode
    movl    %cr0, %eax
    orl     $1, %eax
    movl    %eax, %cr0
    ljmp    $0x8, $(0x8000 + prot_mode_entry - ap_entry)

.align 32
.code32
prot_mode_entry:
    # Enable PAE (Physical Address Extension)
    movl    %cr4, %eax
    orl     $0x20, %eax
    movl    %eax, %cr4

    # Enable long mode by setting the LME bit in the EFER register
    movl    $0xC0000080, %ecx
    rdmsr
    bts     $8, %eax
    wrmsr

    # Load PML4 address into CR3
    movl    0x8000 + ap_pml4 - ap_entry, %eax
    movl    %eax, %cr3

    # Enable paging
    movl    %cr0, %eax
    orl     $(1 << 31), %eax
    movl    %eax, %cr0

    # Set up the data segment registers
    movw    $0x10, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss
    movw    %ax, %fs
    movw    %ax, %gs

    # Far jump to flush the pipeline and switch to long mode
    ljmpl    $0x18, $(0x8000 + long_mode_entry - ap_entry)

.align 16
.code64
long_mode_entry:
    # Check CPU ID
    mov $1, %rax
    cpuid
    shrq $24, %rbx
    movq %rbx, %rdi
    shlq $15, %rbx
    decq %rbx

    # Set up the stack
    movq 0x8000 + ap_stacks - ap_entry, %rsp
    addq %rbx, %rsp

    # Call the callback
    jmpq *(0x8000 + ap_callback - ap_entry)
