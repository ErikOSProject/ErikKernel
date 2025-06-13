#ifndef _TASK_H
#define _TASK_H

#include <erikboot.h>

struct interrupt_frame;
struct process;

struct thread {
	int id;
	struct process *proc;
	bool exiting;

	uintptr_t stack;
	struct list *syscall_params;
	struct interrupt_frame *context;
};

struct process {
	int id;

	struct elf_image *image;
	uint64_t *tables;
	void *syscall_callback;

	struct list *threads;
	int next_tid;

	struct process *parent;
	struct list *children;
};

typedef struct {
	uint64_t cpuid;
	uint64_t kernel_stack;
	uint64_t user_stack;
	struct thread *thread;
} __attribute__((packed)) thread_info;

extern bool scheduler_enabled;

static inline void task_enable_scheduler(bool enable)
{
	scheduler_enabled = enable;
}

void task_init(void);
void task_switch(struct interrupt_frame *frame);
void task_exit(void);
struct thread *task_new_thread(struct process *proc, void *entry);
void task_delete_thread(struct thread *thread);
void task_delete_process(struct process *proc);
struct process *task_find_process(int pid);
struct process *task_fork(struct thread *t);

#endif //_TASK_H
