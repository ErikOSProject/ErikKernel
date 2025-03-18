#include <task.h>
#include <elf.h>
#include <fs.h>
#include <heap.h>
#include <list.h>
#include <memory.h>
#include <paging.h>
#include <spinlock.h>

#include <arch/x86_64/idt.h>
#include <arch/x86_64/msr.h>
#include <arch/x86_64/syscall.h>

#define TASK_DEFAULT_STACK_PAGES 4
#define TASK_DEFAULT_STACK_SIZE (TASK_DEFAULT_STACK_PAGES * PAGE_SIZE)

bool scheduler_enabled = false;
struct list *task_queue = NULL;
struct spinlock task_lock = { 0 };

void task_new_address_space(struct process *proc)
{
	uintptr_t pml4 = (uintptr_t)paging_create_table();
	if (!pml4)
		return;

	paging_clone_higher_half((uint64_t *)tables, (uint64_t *)pml4);
	proc->tables = (uint64_t *)pml4;
}

void task_alloc_stack(struct process *proc)
{
	intptr_t vstack = 0xfffffffff8000000 - TASK_DEFAULT_STACK_SIZE;
	intptr_t stack = find_free_frames(TASK_DEFAULT_STACK_PAGES);
	if (stack == -1)
		return;

	if (set_frame_lock(stack, TASK_DEFAULT_STACK_PAGES, true) < 0)
		return;

	for (size_t i = 0; i < TASK_DEFAULT_STACK_PAGES; i++)
		paging_map_page(proc->tables, vstack + i * PAGE_SIZE,
				stack + i * PAGE_SIZE, P_USER_WRITE);
	proc->stack = vstack;
}

void task_init(void)
{
	spinlock_init(&task_lock);
	spinlock_acquire(&task_lock);
	task_queue = list_create();

	struct process *proc = malloc(sizeof(struct process));
	proc->id = 1;
	task_new_address_space(proc);

	fs_node node;
	fs_find_node(&node, "/init");
	load_elf(&node, proc);
	task_alloc_stack(proc);

	uintptr_t stack = (uintptr_t)malloc(0x1000);
	struct interrupt_frame *frame = malloc(sizeof(struct interrupt_frame));
	frame->rip = proc->entry;
	frame->rsp = proc->stack + TASK_DEFAULT_STACK_SIZE;
	frame->cs = 0x2B;
	frame->ss = 0x23;
	frame->rflags = 0x202;
	proc->context = (void *)frame;

	list_insert_tail(task_queue, proc);
	spinlock_release(&task_lock);
}

void task_switch(struct interrupt_frame *frame)
{
	if (!scheduler_enabled)
		return;

	spinlock_acquire(&task_lock);

	thread_info *info = (thread_info *)read_msr(MSR_GS_BASE);
	if (task_queue && task_queue->length) {
		if (info->proc) {
			*info->proc->context = *frame;
			list_insert_tail(task_queue, info->proc);
		}

		struct node *first_task = list_pop(task_queue);
		if (first_task) {
			struct process *task = first_task->value;
			free(first_task);
			info->proc = task;
			*frame = *task->context;
			paging_set_current(task->tables);
		}
	}

	spinlock_release(&task_lock);
}
