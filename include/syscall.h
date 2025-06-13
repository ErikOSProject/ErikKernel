/**
 * @file syscall.h
 * @brief System call interface.
 *
 * This file contains the declarations for the system call interface.
 * It allows for the invocation of system calls from user space.
 */

#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <task.h>

enum syscall_type {
	SYSCALL_EXIT,
	SYSCALL_METHOD,
	SYSCALL_SIGNAL,
	SYSCALL_TARGETED_SIGNAL,
	SYSCALL_PUSH,
	SYSCALL_PEEK,
	SYSCALL_POP,
};

enum syscall_kernel_interfaces {
	SYSCALL_LOCAL_NAME_SERVICE,
	SYSCALL_GLOBAL_NAME_SERVICE,
	SYSCALL_STDIO,
};

enum syscall_local_name_service_methods {
	SYSCALL_FIND_INTERFACE,
	SYSCALL_FIND_METHOD,
};

enum syscall_global_name_service_methods {
	SYSCALL_FIND_DESTINATION,
	SYSCALL_REGISTER_DESTINATION,
	SYSCALL_UNREGISTER_DESTINATION,
};

enum syscall_stdio_methods {
	SYSCALL_READ,
	SYSCALL_WRITE,
	SYSCALL_FLUSH,
};

enum syscall_param_type {
	SYSCALL_PARAM_ARRAY,
	SYSCALL_PARAM_PRIMITIVE,
};

struct syscall_param {
	enum syscall_param_type type;
	size_t size;
	union {
		uint64_t value;
		void *array;
	};
};

struct syscall_method_data {
	uint64_t pid;
	uint64_t interface;
	uint64_t method;
};

struct syscall_signal_data {
	uint64_t interface;
	uint64_t signal;
};

struct syscall_targeted_signal_data {
	uint64_t pid;
	uint64_t interface;
	uint64_t signal;
};

void syscall_init(void);
void syscall_entry(void);

#endif //_SYSCALL_H
