/**
 * @file paging.h
 * @brief Paging functions for x86_64 architecture.
 *
 * This file contains the declarations for the paging functions for the x86_64 architecture.
 * It allows for the initialization of the paging structures and the setting of page table entries.
 */

#ifndef _ARCH_PAGING_H
#define _ARCH_PAGING_H

#include <paging.h>

#define PML4_INDEX(x) (((x) >> 39) & 0x1FF)
#define PDPT_INDEX(x) (((x) >> 30) & 0x1FF)
#define PD_INDEX(x) (((x) >> 21) & 0x1FF)
#define PT_INDEX(x) (((x) >> 12) & 0x1FF)

#define P_X64_PRESENT (1 << 0)
#define P_X64_WRITE (1 << 1)
#define P_X64_USER (1 << 2)

#define TABLE_DEFAULT (P_X64_PRESENT | P_X64_WRITE | P_X64_USER)

extern uintptr_t ap_pml4;

void get_pml4(void);

#endif //_ARCH_PAGING_H
