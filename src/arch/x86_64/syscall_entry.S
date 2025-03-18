.global syscall_entry
syscall_entry:
    swapgs
    movq %rsp, %gs:0x10 // save user stack pointer
    movq %gs:0x8, %rsp  // switch to kernel stack
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
    movq %rax, %rcx
    call syscall_handler
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
    movq %gs:0x10, %rsp // restore user stack pointer
    swapgs
    sysretq

.global task_idle
task_idle:
    hlt
    jmp task_idle
