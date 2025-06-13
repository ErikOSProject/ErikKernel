/**
 * @file paging.h
 * @brief Paging functions for x86_64 architecture.
 *
 * This file contains the declarations for the paging functions used in the x86_64 architecture.
 */

#ifndef _PAGING_H
#define _PAGING_H

#define P_WRITE (1 << 0)
#define P_USER (1 << 1)
#define P_COW (1 << 2)

#define P_KERNEL_RO 0
#define P_KERNEL_WRITE P_WRITE
#define P_USER_RO P_USER
#define P_USER_WRITE (P_USER | P_WRITE)

extern uint64_t *tables;

void paging_map_page(uint64_t *tables, uintptr_t vaddr, uintptr_t paddr,
		     uint64_t flags);
void paging_unmap_page(uint64_t *tables, uintptr_t vaddr);
uint64_t *paging_create_table(void);
void paging_clone_higher_half(uint64_t *src, uint64_t *dst);
void paging_set_current(uint64_t *tables);

#endif //_PAGING_H
