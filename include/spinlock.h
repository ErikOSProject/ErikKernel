#ifndef _SPINLOCK_H
#define _SPINLOCK_H

#include <erikboot.h>

struct spinlock {
	volatile bool locked;
};

#define spinlock_init(lock)             \
	do {                            \
		(lock)->locked = false; \
	} while (0)
#define spinlock_acquire(lock)                                          \
	do {                                                            \
		while (__sync_lock_test_and_set(&(lock)->locked, true)) \
			;                                               \
	} while (0)
#define spinlock_release(lock)                        \
	do {                                          \
		__sync_lock_release(&(lock)->locked); \
	} while (0)

#endif //_SPINLOCK_H
