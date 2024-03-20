#ifndef MINIOS_KERN_PMAP_H
#define MINIOS_KERN_PMAP_H

#include <type.h>

void	map_kern(phyaddr_t cr3);
void    map_client(phyaddr_t cr3,u32 max_page_addr);
#endif
