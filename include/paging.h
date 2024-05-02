#ifndef _PAGING_H
#define _PAGING_H

#define P_WRITE (1 << 0)
#define P_USER (1 << 1)

#define P_KERNEL_RO 0
#define P_KERNEL_WRITE P_WRITE
#define P_USER_RO P_USER
#define P_USER_WRITE (P_USER | P_WRITE)

extern uint64_t *tables;

void paging_map_page(uint64_t *tables, uintptr_t vaddr, uintptr_t paddr,
		     uint64_t flags);
void paging_unmap_page(uint64_t *tables, uintptr_t vaddr);

#endif //_PAGING_H
