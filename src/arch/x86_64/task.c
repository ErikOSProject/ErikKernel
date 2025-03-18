#include <task.h>
#include <debug.h>
#include <elf.h>
#include <fs.h>
#include <heap.h>
#include <list.h>
#include <memory.h>
#include <paging.h>
#include <spinlock.h>

#include <arch/x86_64/idt.h>
#include <arch/x86_64/msr.h>

#define TASK_DEFAULT_STACK_PAGES 4
#define TASK_DEFAULT_STACK_SIZE (TASK_DEFAULT_STACK_PAGES * PAGE_SIZE)

bool scheduler_enabled = false;
struct list *processes = NULL;
struct list *task_queue = NULL;
struct spinlock task_lock = { 0 };
int task_next_id = 1;

void task_idle(void);
struct interrupt_frame idle_frame = {
	.rip = (uintptr_t)task_idle,
	.cs = 0x08,
	.ss = 0x10,
	.rflags = 0x202,
};

/**
 * @brief Creates a new address space for a process.
 *
 * This function creates a new address space for a process by creating a new
 * Page Map Level 4 (PML4) table and cloning the higher half of the kernel's
 * page tables into it.
 *
 * @param proc A pointer to the process structure.
 */
void task_new_address_space(struct process *proc)
{
	uintptr_t pml4 = (uintptr_t)paging_create_table();
	if (!pml4)
		return;

	paging_clone_higher_half((uint64_t *)tables, (uint64_t *)pml4);
	proc->tables = (uint64_t *)pml4;
}

/**
 * @brief Allocates a stack for a thread.
 *
 * This function allocates a stack for a thread by finding a free region of
 * physical memory and mapping it into the thread's address space.
 *
 * @param t A pointer to the thread structure.
 */
void task_alloc_stack(struct thread *t)
{
	intptr_t vstack = 0xfffffffff8000000 - TASK_DEFAULT_STACK_SIZE * t->id;
	intptr_t stack = find_free_frames(TASK_DEFAULT_STACK_PAGES);
	if (stack == -1)
		return;

	if (set_frame_lock(stack, TASK_DEFAULT_STACK_PAGES, true) < 0)
		return;

	for (size_t i = 0; i < TASK_DEFAULT_STACK_PAGES; i++)
		paging_map_page(t->proc->tables, vstack + i * PAGE_SIZE,
				stack + i * PAGE_SIZE, P_USER_WRITE);
	t->stack = vstack;
}

/**
 * @brief Initializes the task scheduler.
 *
 * This function initializes the task scheduler by creating the process list
 * and the task queue, and creating the initial process and thread.
 */
void task_init(void)
{
	spinlock_init(&task_lock);
	spinlock_acquire(&task_lock);
	processes = list_create();
	task_queue = list_create();

	struct process *proc = malloc(sizeof(struct process));
	proc->id = task_next_id++;

	task_new_address_space(proc);
	fs_node node;
	fs_find_node(&node, "/init");
	load_elf(&node, proc);

	proc->syscall_callback = NULL;
	proc->threads = list_create();
	proc->next_tid = 1;
	proc->parent = NULL;
	proc->children = list_create();

	task_new_thread(proc, (void *)proc->image->entry);
	list_insert_tail(processes, proc);
	spinlock_release(&task_lock);
}

/**
 * @brief Switches to the next task in the task queue.
 *
 * This function switches to the next task in the task queue by saving the
 * current task's context and restoring the next task's context.
 *
 * @param frame A pointer to the interrupt frame.
 */
void task_switch(struct interrupt_frame *frame)
{
	if (!scheduler_enabled)
		return;

	spinlock_acquire(&task_lock);

	thread_info *info = (thread_info *)read_msr(MSR_GS_BASE);

	if (info->thread && info->thread->exiting) {
		task_delete_thread(info->thread);
		info->thread = NULL;
	}

	if (task_queue && task_queue->length) {
		if (info->thread) {
			if (info->thread->exiting) {
				task_delete_thread(info->thread);
				info->thread = NULL;
			} else {
				*info->thread->context = *frame;
				list_insert_tail(task_queue, info->thread);
			}
		}

		struct thread *task = list_pop(task_queue);
		if (task) {
			info->thread = task;
			*frame = *task->context;
			paging_set_current(task->proc->tables);
		}
	} else if (!info->thread)
		*frame = idle_frame;

	spinlock_release(&task_lock);
}

/**
 * @brief Marks the current task as exiting.
 */
void task_exit(void)
{
	thread_info *info = (thread_info *)read_msr(MSR_GS_BASE);
	info->thread->exiting = true;
}

/**
 * @brief Creates a new thread for a process.
 *
 * This function creates a new thread for a process by allocating a new thread
 * structure, allocating a stack for the thread, and initializing the thread's
 * context.
 *
 * @param proc A pointer to the process structure.
 * @param entry The entry point for the thread.
 *
 * @return A pointer to the new thread structure.
 */
struct thread *task_new_thread(struct process *proc, void *entry)
{
	struct thread *thread = malloc(sizeof(struct thread));
	thread->id = proc->next_tid++;
	thread->proc = proc;
	thread->exiting = false;
	thread->syscall_params = list_create();

	task_alloc_stack(thread);
	struct interrupt_frame *frame = malloc(sizeof(struct interrupt_frame));
	frame->rip = (uintptr_t)entry;
	frame->rsp = thread->stack + TASK_DEFAULT_STACK_SIZE;
	frame->rbp = frame->rsp;
	frame->cs = 0x2B;
	frame->ss = 0x23;
	frame->rflags = 0x202;
	thread->context = (void *)frame;

	list_insert_tail(proc->threads, thread);
	list_insert_tail(task_queue, thread);
	return thread;
}

/**
 * @brief Deletes a thread.
 *
 * This function deletes a thread by removing it from the task queue and the
 * process's thread list, unlocking the thread's stack, and freeing the thread's
 * context and structure.
 *
 * @param thread A pointer to the thread structure.
 */
void task_delete_thread(struct thread *thread)
{
	struct node *n = list_find(task_queue, thread);
	if (n)
		list_delete(task_queue, n);
	n = list_find(thread->proc->threads, thread);
	if (n)
		list_delete(thread->proc->threads, n);
	set_frame_lock(thread->stack, TASK_DEFAULT_STACK_PAGES, false);
	list_destroy(thread->syscall_params);
	free(thread->context);
	free(thread);
}

/**
 * @brief Deletes a process.
 *
 * This function deletes a process by removing it from the process list and
 * deleting all of its threads and child processes.
 *
 * @param proc A pointer to the process structure.
 */
void task_delete_process(struct process *proc)
{
	struct node *n = list_find(processes, proc);
	if (n)
		list_delete(processes, n);

	while (proc->threads->length) {
		struct thread *thread = list_pop(proc->threads);
		task_delete_thread(thread);
	}
	list_destroy(proc->threads);

	while (proc->children->length) {
		struct process *child = list_pop(proc->children);
		task_delete_process(child);
	}
	list_destroy(proc->children);

	//TODO: free address space
	free(proc);
}

/**
 * @brief Finds a process based on the process ID.
 *
 * This function finds a process based on the process ID.
 *
 * @param pid The process ID.
 *
 * @return A pointer to the process structure, or NULL if the process was not found.
 */
struct process *task_find_process(int pid)
{
	for (struct node *n = processes->head; n; n = n->next) {
		struct process *proc = (struct process *)n->value;
		if (proc->id == pid)
			return proc;
	}
	return NULL;
}
