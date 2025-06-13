/**
 * @file syscall.c
 * @brief System call implementation for x86_64 architecture.
 *
 * This file contains the implementation of the system call interface for the x86_64 architecture.
 * It allows for the invocation of system calls from user space.
 */

#include <debug.h>
#include <heap.h>
#include <list.h>
#include <memory.h>
#include <paging.h>
#include <task.h>
#include <syscall.h>

#include <arch/x86_64/idt.h>
#include <arch/x86_64/msr.h>

struct syscall_id_name {
	int64_t id;
	const char *name;
};

static struct syscall_id_name syscall_kernel = { 0, "fi.erikinkinen.kernel" };

static struct syscall_id_name syscall_kernel_interfaces[] = {
	{ SYSCALL_LOCAL_NAME_SERVICE,
	  "fi.erikinkinen.LocalNameService" },
	{ SYSCALL_GLOBAL_NAME_SERVICE,
	  "fi.erikinkinen.GlobalNameService" },
	{ SYSCALL_STDIO, "fi.erikinkinen.kernel.Stdio" },
	{ 0, NULL },
};

static struct syscall_id_name syscall_local_name_service_methods[] = {
	{ SYSCALL_FIND_INTERFACE, "FindInterface" },
	{ SYSCALL_FIND_METHOD, "FindMethod" },
	{ 0, NULL },
};

static struct syscall_id_name syscall_global_name_service_methods[] = {
	{ SYSCALL_FIND_DESTINATION, "FindDestination" },
	{ SYSCALL_REGISTER_DESTINATION, "RegisterDestination" },
	{ SYSCALL_UNREGISTER_DESTINATION, "UnregisterDestination" },
	{ 0, NULL },
};

static struct syscall_id_name syscall_stdio_methods[] = {
	{ SYSCALL_READ, "Read" },
	{ SYSCALL_WRITE, "Write" },
	{ SYSCALL_FLUSH, "Flush" },
	{ 0, NULL },
};

struct list *syscall_services = NULL;

/**
 * @brief Finds a service based on the service name.
 *
 * This function finds a service based on the service name.
 *
 * @param params A pointer to the system call parameter list.
 *
 * @return The service ID on success, a negative error code on failure.
 */
static int64_t syscall_find_service(struct list *params)
{
	struct syscall_param *name_param =
		(struct syscall_param *)list_shift(params);
	if (!name_param || name_param->type != SYSCALL_PARAM_ARRAY)
		return -1;
	const char *name = name_param->array;
	if (!name)
		return -1;

	for (struct node *node = syscall_services->head; node;
	     node = node->next) {
		struct syscall_id_name *service =
			(struct syscall_id_name *)node->value;
		if (strcmp(name, service->name) == 0)
			return service->id;
	}

	return -1;
}

/**
 * @brief Finds a system call id for a given name.
 *
 * @param params A pointer to the system call parameter list.
 * @param table A pointer to the system call id table.
 *
 * @return The id on success, a negative error code on failure.
 */
static int64_t syscall_find_id(struct list *params,
			       struct syscall_id_name *table)
{
	struct syscall_param *name_param =
		(struct syscall_param *)list_shift(params);
	if (!name_param || name_param->type != SYSCALL_PARAM_ARRAY)
		return -1;
	const char *name = name_param->array;
	if (!name)
		return -1;

	for (size_t i = 0; table[i].name; i++) {
		if (!strcmp(name, table[i].name)) {
			return table[i].id;
		}
	}
	return -1;
}

/**
 * @brief Finds a method based on the interface identifier and method name.
 *
 * @param params A pointer to the system call parameter list.
 *
 * @return The method identifier on success, a negative error code on failure.
 */
static int64_t syscall_find_method(struct list *params)
{
	struct syscall_param *interface_param =
		(struct syscall_param *)list_shift(params);
	if (!interface_param ||
	    interface_param->type != SYSCALL_PARAM_PRIMITIVE)
		return -1;
	int64_t interface = interface_param->value;

	switch (interface) {
	case SYSCALL_LOCAL_NAME_SERVICE:
		return syscall_find_id(params,
				       syscall_local_name_service_methods);
	case SYSCALL_GLOBAL_NAME_SERVICE:
		return syscall_find_id(params,
				       syscall_global_name_service_methods);
	case SYSCALL_STDIO:
		return syscall_find_id(params, syscall_stdio_methods);
	default:
		return -1;
	}
}

/**
 * @brief Writes a string to the standard output.
 *
 * This function writes a string to the standard output.
 *
 * @param params A pointer to the system call parameter list.
 *
 * @return Zero on success, a negative error code on failure.
 */
static int64_t syscall_stdio_write(struct list *params)
{
	struct syscall_param *str_param =
		(struct syscall_param *)list_shift(params);
	if (!str_param || str_param->type != SYSCALL_PARAM_ARRAY)
		return -1;
	const char *str = str_param->array;
	if (!str)
		return -1;

	DEBUG_PRINTF("%s", str);
	return 0;
}

/**
 * @brief Registers a service with the global name service.
 *
 * This function registers a service with the global name service.
 *
 * @param params A pointer to the system call parameter list.
 *
 * @return The ID of the registered service on success, a negative error code on failure.
 */
static int64_t syscall_register_service(struct list *params)
{
	thread_info *info = (thread_info *)read_msr(MSR_GS_BASE);

	struct syscall_param *name_param =
		(struct syscall_param *)list_shift(params);
	if (!name_param || name_param->type != SYSCALL_PARAM_ARRAY)
		return -1;
	const char *name = name_param->array;
	if (!name)
		return -1;

	struct syscall_param *callback_param =
		(struct syscall_param *)list_shift(params);
	if (!callback_param || callback_param->type != SYSCALL_PARAM_PRIMITIVE)
		return -1;
	void *callback = callback_param->array;
	if (!callback)
		return -1;

	struct syscall_id_name *service = NULL;
	for (struct node *node = syscall_services->head; node;
	     node = node->next) {
		struct syscall_id_name *s =
			(struct syscall_id_name *)node->value;
		if (s->id == info->thread->proc->id) {
			service = s;
			break;
		}
	}

	if (!service) {
		service = malloc(sizeof(struct syscall_id_name));
		service->id = info->thread->proc->id;
		list_insert_tail(syscall_services, service);
	}

	service->name = name;
	info->thread->proc->syscall_callback = callback;

	return service->id;
}

/**
 * @brief Unregisters a service from the global name service.
 *
 * This function unregisters a service from the global name service.
 *
 * @param params A pointer to the system call parameter list.
 *
 * @return Zero on success, a negative error code on failure.
 */
static int64_t syscall_unregister_service(struct list *params)
{
	struct syscall_param *name_param =
		(struct syscall_param *)list_shift(params);
	if (!name_param || name_param->type != SYSCALL_PARAM_ARRAY)
		return -1;
	const char *name = name_param->array;
	if (!name)
		return -1;

	for (struct node *node = syscall_services->head; node;
	     node = node->next) {
		struct syscall_id_name *service =
			(struct syscall_id_name *)node->value;
		if (strcmp(name, service->name) == 0) {
			free(service);
			list_delete(syscall_services, node);
			return 0;
		}
	}

	return -1;
}

/**
 * @brief Kernel system call method handler.
 *
 * This function is responsible for handling system calls to the kernel. It dispatches the
 * system call to the appropriate handler based on the interface and method identifiers.
 *
 * @param interface The interface identifier.
 * @param method The method identifier.
 * @param params A pointer to the system call parameter list.
 *
 * @return The return value of the system call method.
 */
static int64_t syscall_kernel_method(uint64_t interface, uint64_t method,
				     struct list *params)
{
	switch (interface) {
	case SYSCALL_LOCAL_NAME_SERVICE:
		switch (method) {
		case SYSCALL_FIND_INTERFACE:
			return syscall_find_id(params,
					       syscall_kernel_interfaces);
		case SYSCALL_FIND_METHOD:
			return syscall_find_method(params);
		}
		return -1;
	case SYSCALL_GLOBAL_NAME_SERVICE:
		switch (method) {
		case SYSCALL_FIND_DESTINATION:
			return syscall_find_service(params);
		case SYSCALL_REGISTER_DESTINATION:
			return syscall_register_service(params);
		case SYSCALL_UNREGISTER_DESTINATION:
			return syscall_unregister_service(params);
		}
		return -1;
	case SYSCALL_STDIO:
		switch (method) {
		case SYSCALL_READ:
			return -1;
		case SYSCALL_WRITE:
			return syscall_stdio_write(params);
		case SYSCALL_FLUSH:
			return 0;
		}
		return -1;
	default:
		return -1;
	}
}

/**
 * @brief Copy system call parameters.
 *
 * This function copies system call parameters from the source list to the destination list.
 *
 * @param src A pointer to the source list.
 * @param dst A pointer to the destination list.
 */
static void syscall_copy_params(struct list *src, struct list *dst)
{
	while (src->length) {
		struct syscall_param *param =
			(struct syscall_param *)list_shift(src);
		list_insert_tail(dst, param);
	}
}

/**
 * @brief Copy a parameter from the system call parameter list.
 *
 * This function copies a parameter from the system call parameter list to the destination
 * parameter structure.
 *
 * @param src A pointer to the source parameter structure.
 * @param dst A pointer to the destination parameter structure.
 *
 * @return Zero on success, a negative error code on failure.
 */
static int64_t syscall_copy_param(struct syscall_param *src,
				  struct syscall_param *dst)
{
	dst->type = src->type;
	dst->size = src->size;
	if (src->type == SYSCALL_PARAM_ARRAY) {
		if (!dst->array)
			return 0;
		if ((uintptr_t)dst->array < 0xFFFFFFFFF8000000) {
			memcpy(dst->array, src->array, src->size);
		} else
			return -1; // Do not allow copying to kernel space
	} else
		dst->value = src->value;
	return 0;
}

/**
 * @brief Push a parameter onto the system call parameter list.
 * 
 * This function pushes a parameter onto the system call parameter list.
 * 
 * @param params A pointer to the system call parameter list.
 * @param data A pointer to the parameter structure to be pushed.
 * 
 * @return Zero on success, a negative error code on failure.
 */
static int64_t syscall_param_push(struct list *params, void *data)
{
	struct syscall_param *param = (struct syscall_param *)data;
	if (!param)
		return -1;
	struct syscall_param *copy = malloc(sizeof(struct syscall_param));
	if (!copy)
		return -1;
	copy->type = param->type;
	copy->size = param->size;
	if (param->type == SYSCALL_PARAM_ARRAY) {
		if ((uintptr_t)param->array >= 0xFFFFFFFFF8000000)
			return -1; // Do not allow copying from kernel space
		copy->array = malloc(param->size);
		if (!copy->array) {
			free(copy);
			return -1;
		}
		memcpy(copy->array, param->array, param->size);
	} else {
		copy->value = param->value;
	}
	list_insert_tail(params, copy);
	return 0;
}

/**
 * @brief Peek a parameter from the system call parameter list.
 *
 * This function peeks a parameter from the system call parameter list and copies it to the
 * destination parameter structure.
 *
 * @param params A pointer to the system call parameter list.
 * @param data A pointer to the destination parameter structure.
 *
 * @return Zero on success, a negative error code on failure.
 */
static int64_t syscall_param_peek(struct list *params, void *data)
{
	struct syscall_param *dst = (struct syscall_param *)data;
	struct syscall_param *src = (struct syscall_param *)params->head->value;
	if (!src)
		return -1;
	return syscall_copy_param(src, dst);
}

/**
 * @brief Pop a parameter from the system call parameter list.
 *
 * This function pops a parameter from the system call parameter list and copies it to the
 * destination parameter structure.
 *
 * @param params A pointer to the system call parameter list.
 * @param data A pointer to the destination parameter structure.
 *
 * @return Zero on success, a negative error code on failure.
 */
static int64_t syscall_param_pop(struct list *params, void *data)
{
	struct syscall_param *dst = (struct syscall_param *)data;
	struct syscall_param *src = (struct syscall_param *)list_shift(params);
	if (!src)
		return -1;
	return syscall_copy_param(src, dst);
}

/**
 * @brief System call method handler.
 *
 * This function is responsible for handling system call methods. It dispatches the
 * system call to the appropriate handler based on the interface and method identifiers.
 *
 * @param data A pointer to the system call method data structure.
 * @param info A pointer to the thread information structure.
 *
 * @return The return value of the system call method.
 */
static int64_t syscall_method(struct syscall_method_data *data,
			      thread_info *info)
{
	if (!data)
		return -1;

	struct list *params = info->thread->syscall_params;
	if (data->pid == 0)
		return syscall_kernel_method(data->interface, data->method,
					     params);
	else {
		struct process *proc = task_find_process(data->pid);
		if (!proc || !proc->syscall_callback)
			return -1;
		paging_set_current(tables);
		struct thread *t =
			task_new_thread(proc, (void *)proc->syscall_callback);
		paging_set_current(info->thread->proc->tables);
		t->context->rdi = data->interface;
		t->context->rsi = data->method;
		syscall_copy_params(params, t->syscall_params);
	}
	return -1;
}

/**
 * @brief System call entry point.
 *
 * This function is the C entry point for system calls. It is called by the assembly
 * stub that is invoked by the SYSCALL instruction.
 * 
 * @param frame A pointer to the interrupt frame structure.
 */
void syscall_handler(struct interrupt_frame *frame)
{
	enum syscall_type type = frame->rdi;
	void *data = (void *)frame->rsi;
	thread_info *info = (thread_info *)read_msr(MSR_GS_BASE);
	struct list *params = info->thread->syscall_params;
	switch (type) {
	case SYSCALL_EXIT:
		task_exit();
		task_switch(frame);
		break;
	case SYSCALL_METHOD:
		frame->rax = syscall_method((struct syscall_method_data *)data,
					    info);
		break;
	case SYSCALL_PUSH:
		frame->rax = syscall_param_push(params, data);
		break;
	case SYSCALL_PEEK:
		frame->rax = syscall_param_peek(params, data);
		break;
	case SYSCALL_POP:
		frame->rax = syscall_param_pop(params, data);
		break;
	default:
		frame->rax = -1;
		break;
	}
}

/**
 * @brief Initialize the system call interface.
 *
 * This function initializes the system call interface by enabling the syscall/sysret
 * instructions and setting the appropriate model-specific registers (MSRs).
 */
void syscall_init(void)
{
	// Enable syscall/sysret instructions
	uint64_t efer = read_msr(MSR_EFER);
	efer |= EFER_SCE;
	write_msr(MSR_EFER, efer);

	// Set syscall/sysret CS and SS selectors
	write_msr(MSR_STAR, ((uint64_t)0x8 << 32) | ((uint64_t)0x18 << 48));
	write_msr(MSR_LSTAR, (uint64_t)syscall_entry);
	write_msr(MSR_SFMASK, 0x700);

	// Initialize syscall services list
	syscall_services = list_create();
	list_insert_tail(syscall_services, &syscall_kernel);
}
