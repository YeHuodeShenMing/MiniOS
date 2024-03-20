#ifndef MINIOS_KERN_PMAP_H
#define MINIOS_KERN_PMAP_H

#include <type.h>
#include <kern/process.h>

void	map_kern(u32 cr3, page_node_t **page_list);
void	map_elf(pcb_t *p_proc, void *elf_addr);
void	map_stack(pcb_t *p_proc);
void	recycle_pages(page_node_t *page_list);
void    lin_mapping_phy(u32			cr3,
			page_node_t		**page_list,
			uintptr_t		laddr,
			phyaddr_t		paddr,
			u32			pte_flag);
#endif /* MINIOS_KERN_PMAP_H */