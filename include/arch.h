/**
 * @file arch.h
 * @brief Architecture-specific functions.
 *
 * This file contains the declarations for generic functions that are
 * implemented in an architecture-specific manner.
 */

#ifndef _ARCH_H
#define _ARCH_H

#include <erikboot.h>

void arch_init(BootInfo *boot_info);

#endif //_ARCH_H
