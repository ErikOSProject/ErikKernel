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

#endif //_TASK_H
