#ifndef _TASK_H
#define _TASK_H

#include <erikboot.h>

struct interrupt_frame;

struct process {
	int id;

	struct interrupt_frame *context;
	uint64_t *tables;

	uintptr_t entry;
	uintptr_t stack;
};

extern bool scheduler_enabled;

static inline void task_enable_scheduler(bool enable)
{
	scheduler_enabled = enable;
}

void task_init(void);
void task_switch(struct interrupt_frame *frame);

#endif //_TASK_H
