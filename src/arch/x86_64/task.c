#include <task.h>
#include <debug.h>
#include <elf.h>
#include <fs.h>
#include <heap.h>
#include <list.h>
#include <memory.h>
#include <paging.h>
#include <arch/x86_64/paging.h>
#include <spinlock.h>
#include <syscall.h>

#include <arch/x86_64/idt.h>
#include <arch/x86_64/msr.h>

extern memory _memory;

#define TASK_DEFAULT_STACK_PAGES 4
#define TASK_DEFAULT_STACK_SIZE (TASK_DEFAULT_STACK_PAGES * PAGE_SIZE)
#define KERNEL_BASE 0xfffffffff8000000

/**
 * @brief Release all mappings and page tables of an address space.
 *
 * Traverses the user portion of the provided PML4 and releases every page
 * mapping and page table. Physical frames are unlocked once their reference
 * count drops to zero. The kernel half of the tables is also cleaned up.
 *
 * @param pml4 Pointer to the root page table of the address space.
 */
static void task_free_address_space(uint64_t *pml4)
{
	if (!pml4)
		return;

	for (size_t pml4_i = 0; pml4_i < 512; pml4_i++) {
		uintptr_t addr1 = (uintptr_t)pml4_i << 39;
		if (addr1 >= KERNEL_BASE)
			break;

		if (!(pml4[pml4_i] & P_X64_PRESENT))
			continue;

		uint64_t *pdpt = (uint64_t *)(pml4[pml4_i] & ~0xFFF);
		for (size_t pdpt_i = 0; pdpt_i < 512; pdpt_i++) {
			uintptr_t addr2 = addr1 | ((uintptr_t)pdpt_i << 30);
			if (addr2 >= KERNEL_BASE)
				break;
			if (!(pdpt[pdpt_i] & P_X64_PRESENT))
				continue;

			uint64_t *pd = (uint64_t *)(pdpt[pdpt_i] & ~0xFFF);
			for (size_t pd_i = 0; pd_i < 512; pd_i++) {
				uintptr_t addr3 = addr2 |
						  ((uintptr_t)pd_i << 21);
				if (addr3 >= KERNEL_BASE)
					break;
				if (!(pd[pd_i] & P_X64_PRESENT))
					continue;

				uint64_t *pt = (uint64_t *)(pd[pd_i] & ~0xFFF);
				for (size_t pt_i = 0; pt_i < 512; pt_i++) {
					uint64_t entry = pt[pt_i];
					if (!(entry & P_X64_PRESENT))
						continue;
					uintptr_t paddr = entry & ~0xFFF;
					frame_ref_dec(paddr);
					if (!frame_refcounts) {
						set_frame_lock(paddr, 1, false);
					} else {
						size_t idx =
							(paddr - _memory.base) /
							PAGE_SIZE;
						if (frame_refcounts[idx] == 0)
							set_frame_lock(paddr, 1,
								       false);
					}
				}
				set_frame_lock((uintptr_t)pt, 1, false);
			}
			set_frame_lock((uintptr_t)pd, 1, false);
		}
		set_frame_lock((uintptr_t)pdpt, 1, false);
	}

	if (pml4[0x1ff] & P_X64_PRESENT) {
		uint64_t *pdpt = (uint64_t *)(pml4[0x1ff] & ~0xFFF);
		if (pdpt[0x1ff] & P_X64_PRESENT) {
			uint64_t *pd = (uint64_t *)(pdpt[0x1ff] & ~0xFFF);
			set_frame_lock((uintptr_t)pd, 1, false);
		}
		set_frame_lock((uintptr_t)pdpt, 1, false);
	}
	set_frame_lock((uintptr_t)pml4, 1, false);
}

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

	task_new_thread(proc, (void *)proc->image->entry, false);
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
struct thread *task_new_thread(struct process *proc, void *entry,
			       bool ipc_handler)
{
	struct thread *thread = malloc(sizeof(struct thread));
	thread->id = proc->next_tid++;
	thread->proc = proc;
	thread->exiting = false;
	thread->ipc_handler = ipc_handler;
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

	if (!thread->ipc_handler) {
		while (thread->syscall_params->length) {
			struct syscall_param *param =
				list_shift(thread->syscall_params);
			if (param->type == SYSCALL_PARAM_ARRAY)
				free(param->array);
			free(param);
		}
		list_destroy(thread->syscall_params);
	}

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

	if (proc->tables) {
		task_free_address_space(proc->tables);
		proc->tables = NULL;
	}

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

/**
 * @brief Clones the user space of a process with copy-on-write semantics.
 *
 * This function clones the user space of a process by creating new page tables
 * and marking writable pages as copy-on-write. It ensures that modifications to
 * these pages in either the parent or child process do not affect the other.
 *
 * @param src Pointer to the source process's page tables.
 * @param dst Pointer to the destination process's page tables.
 */
static void clone_user_space_cow(uint64_t *src, uint64_t *dst)
{
	for (size_t pml4_i = 0; pml4_i < 512; pml4_i++) {
		if (!(src[pml4_i] & P_X64_PRESENT))
			continue;
		uint64_t base = (uint64_t)pml4_i << 39;
		if (base >= KERNEL_BASE)
			break;

		uint64_t *src_pdpt = (uint64_t *)(src[pml4_i] & ~0xFFF);
		uint64_t *dst_pdpt = paging_create_table();
		dst[pml4_i] = ((uint64_t)dst_pdpt) | TABLE_DEFAULT;

		for (size_t pdpt_i = 0; pdpt_i < 512; pdpt_i++) {
			if (!(src_pdpt[pdpt_i] & P_X64_PRESENT))
				continue;
			uint64_t addr1 = base | ((uint64_t)pdpt_i << 30);
			if (addr1 >= KERNEL_BASE)
				break;

			uint64_t *src_pd =
				(uint64_t *)(src_pdpt[pdpt_i] & ~0xFFF);
			uint64_t *dst_pd = paging_create_table();
			dst_pdpt[pdpt_i] = ((uint64_t)dst_pd) | TABLE_DEFAULT;

			for (size_t pd_i = 0; pd_i < 512; pd_i++) {
				if (!(src_pd[pd_i] & P_X64_PRESENT))
					continue;
				uint64_t addr2 = addr1 | ((uint64_t)pd_i << 21);
				if (addr2 >= KERNEL_BASE)
					break;

				uint64_t *src_pt =
					(uint64_t *)(src_pd[pd_i] & ~0xFFF);
				uint64_t *dst_pt = paging_create_table();
				dst_pd[pd_i] = ((uint64_t)dst_pt) |
					       TABLE_DEFAULT;

				for (size_t pt_i = 0; pt_i < 512; pt_i++) {
					uint64_t entry = src_pt[pt_i];
					if (!(entry & P_X64_PRESENT))
						continue;
					uint64_t vaddr = addr2 |
							 ((uint64_t)pt_i << 12);
					if (vaddr >= KERNEL_BASE)
						break;

					uint64_t flags = entry & 0xFFF;
					uintptr_t paddr = entry & ~0xFFF;
					if (flags & P_X64_WRITE) {
						flags &= ~P_X64_WRITE;
						flags |= P_X64_COW;
						src_pt[pt_i] = paddr | flags;
					}
					dst_pt[pt_i] = paddr | flags;
					frame_ref_inc(paddr);
				}
			}
		}
	}
}

/**
 * @brief Fork a thread into a new process.
 *
 * This creates a new child process by cloning the parent's address space.
 * Writable pages are marked copy-on-write so they are only duplicated
 * when modified by either process. The provided thread is duplicated so
 * execution in the child continues from the same point as the parent.
 *
 * @param thread Pointer to the thread to be forked.
 *
 * @return A pointer to the newly created process or NULL on failure.
 */
struct process *task_fork(struct thread *thread)
{
	if (!thread)
		return NULL;

	struct process *parent = thread->proc;

	spinlock_acquire(&task_lock);

	struct process *child = malloc(sizeof(struct process));
	if (!child) {
		spinlock_release(&task_lock);
		return NULL;
	}

	child->id = task_next_id++;
	child->image = parent->image;
	if (child->image)
		child->image->refcount++;

	task_new_address_space(child);
	clone_user_space_cow(parent->tables, child->tables);
	child->syscall_callback = parent->syscall_callback;

	child->threads = list_create();
	child->next_tid = 1;
	child->parent = parent;
	child->children = list_create();
	list_insert_tail(parent->children, child);

	struct thread *child_thread = malloc(sizeof(struct thread));
	child_thread->id = child->next_tid++;
	child_thread->proc = child;
	child_thread->exiting = false;
	child_thread->ipc_handler = false;
	child_thread->stack = thread->stack;
	child_thread->syscall_params = list_create();
	child_thread->context = malloc(sizeof(struct interrupt_frame));
	*child_thread->context = *thread->context;

	list_insert_tail(child->threads, child_thread);
	list_insert_tail(task_queue, child_thread);
	list_insert_tail(processes, child);

	spinlock_release(&task_lock);
	return child;
}

/**
 * @brief Replaces the current thread's process image with a new executable.
 *
 * This function loads a new executable from the specified path into the address space
 * of the process associated with the given thread. It performs the following steps:
 *   - Validates the input thread and path.
 *   - Finds the filesystem node for the executable.
 *   - Acquires the task lock to ensure thread safety.
 *   - Deletes all other threads in the process except the current one.
 *   - Resets thread IDs and initializes syscall parameters.
 *   - Allocates and sets up a new stack for the thread.
 *   - Frees and creates a new address space for the process.
 *   - Loads the ELF executable into the process address space.
 *   - Initializes the thread's context to start execution at the new entry point.
 *   - Sets the current paging tables to the new address space.
 *   - Releases the task lock.
 *
 * @param thread Pointer to the thread whose process image will be replaced.
 * @param path   Path to the executable file to load.
 * @return 0 on success, -1 on failure.
 */
int task_exec(struct thread *thread, const char *path)
{
	if (!thread || !path)
		return -1;

	fs_node node;
	if (fs_find_node(&node, path) < 0)
		return -1;

	spinlock_acquire(&task_lock);

	struct process *proc = thread->proc;

	struct node *n = proc->threads->head;
	while (n) {
		struct thread *t = (struct thread *)n->value;
		n = n->next;
		if (t != thread)
			task_delete_thread(t);
	}

	proc->next_tid = 1;
	thread->id = proc->next_tid++;

	list_destroy(thread->syscall_params);
	thread->syscall_params = list_create();

	set_frame_lock(thread->stack, TASK_DEFAULT_STACK_PAGES, false);
	task_alloc_stack(thread);

	task_free_address_space(proc->tables);
	task_new_address_space(proc);

	if (!load_elf(&node, proc)) {
		spinlock_release(&task_lock);
		return -1;
	}

	free(thread->context);
	thread->context = malloc(sizeof(struct interrupt_frame));
	thread->context->rip = proc->image->entry;
	thread->context->rsp = thread->stack + TASK_DEFAULT_STACK_SIZE;
	thread->context->rbp = thread->context->rsp;
	thread->context->cs = 0x2B;
	thread->context->ss = 0x23;
	thread->context->rflags = 0x202;

	paging_set_current(proc->tables);

	spinlock_release(&task_lock);
	return 0;
}
