#include <assert.h>
#include <elf.h>
#include <string.h>
#include <mmu.h>

#include <kern/stdio.h>
#include <kern/kmalloc.h>
#include <kern/pmap.h>
#include <kern/stdio.h>
/*
 * 申请一个新的物理页，并更新page_list页面信息
 * 返回新申请的物理页面的物理地址
 */
static phyaddr_t
alloc_phy_page(struct page_node **page_list)
{
	phyaddr_t paddr = phy_malloc_4k();//返回分配的物理页首地址
	
	struct page_node *new_node = kmalloc(sizeof(struct page_node));//分配新页结点
	new_node->nxt = *page_list;//在前面增加页结点
	new_node->paddr = paddr;
	new_node->laddr = -1;
	*page_list = new_node;//指向新页结点

	return paddr;//返回新的物理页首地址
}

/*
 * MINIOS中比较通用的页表映射函数
 * 它将laddr处的虚拟页面映射到物理地址为paddr（如果paddr为-1则会自动申请一个新的物理页面）的物理页面
 * 并将pte_flag置位到页表项（页目录项标志位默认为PTE_P | PTE_W | PTE_U）
 * 这个函数中所有新申请到的页面信息会存放到page_list这个链表中
 */
// static void
void
lin_mapping_phy(u32			cr3,
		struct page_node	**page_list,
		uintptr_t		laddr,
		phyaddr_t		paddr,
		u32			pte_flag)
{
	assert(PGOFF(laddr) == 0);
	//将页目录首地址转化为虚拟地址
	uintptr_t *pde_ptr = (uintptr_t *)K_PHY2LIN(cr3);
	//如果该虚拟地址对应的页目录项不存在，
	if ((pde_ptr[PDX(laddr)] & PTE_P) == 0) {//PDX(laddr)：要转化的线性地址的页目录索引，PTE_P=1页面存在
		phyaddr_t pte_phy = alloc_phy_page(page_list);//分配一个物理页，此物理页为虚拟地址所在页表
		memset((void *)K_PHY2LIN(pte_phy), 0, PGSIZE);
		pde_ptr[PDX(laddr)] = pte_phy | PTE_P | PTE_W | PTE_U;//在页目录项相应位置填上页表物理首地址
	}

	phyaddr_t pte_phy = PTE_ADDR(pde_ptr[PDX(laddr)]);//将页目录项（页表首地址）的低12位抹0，
	uintptr_t *pte_ptr = (uintptr_t *)K_PHY2LIN(pte_phy);//转化为线性地址

	phyaddr_t page_phy;
	if (paddr == (phyaddr_t)-1) {//将-1作数据类型转化，不关心分配的物理地址是多少
		if ((pte_ptr[PTX(laddr)] & PTE_P) != 0)//如果该页面已登记在页表项中，就返回
			return;
		page_phy = alloc_phy_page(page_list);	//分配物理页
		(*page_list)->laddr = laddr;		//头节点为新页节点，所以此时可将线性地址laddr重新赋过去
		
	} else {
		if ((pte_ptr[PTX(laddr)] & PTE_P) != 0)//先前曾经分配过该页面
			warn("this page was mapped before, laddr: %x", laddr);
		assert(PGOFF(paddr) == 0);
		page_phy = paddr;
	}
	pte_ptr[PTX(laddr)] = page_phy | pte_flag;//将物理页登记在页表中
}

/*
 * 初始化进程页表的内核部分
 * 将3GB ~ 3GB + 128MB的线性地址映射到0 ~ 128MB的物理地址
 */
void
map_kern(u32 cr3, struct page_node **page_list)
{
	for (phyaddr_t paddr = 0 ; paddr < 128 * MB ; paddr += PGSIZE) {
		lin_mapping_phy(cr3,
				page_list,
				K_PHY2LIN(paddr),
				paddr,
				PTE_P | PTE_W | PTE_U);
	}
}

/*
 * 根据elf文件信息将数据搬迁到指定位置
 * 中间包含了对页表的映射，eip的置位
 * 这也是你们做实验五中最折磨的代码，可以看这份学习一下
 */
void
map_elf(PROCESS_0 *p_proc, void *elf_addr)
{
	assert(p_proc->lock != 0);

	struct Elf *eh = (struct Elf *)elf_addr;

	struct Proghdr *ph = (struct Proghdr *)(elf_addr + eh->e_phoff);
	for (int i = 0 ; i < eh->e_phnum ; i++, ph++) {
		if (ph->p_type != PT_LOAD)
			continue;
		uintptr_t st = ROUNDDOWN(ph->p_va, PGSIZE);
		uintptr_t en = ROUNDUP(st + ph->p_memsz, PGSIZE);
		for (uintptr_t laddr = st ; laddr < en ; laddr += PGSIZE) {
			u32 pte_flag = PTE_P | PTE_U;
			if ((ph->p_flags & ELF_PROG_FLAG_WRITE) != 0)
				pte_flag |= PTE_W;
			lin_mapping_phy(p_proc->cr3,
					&p_proc->page_list,
					laddr,
					(phyaddr_t)-1,
					pte_flag);
		}
		memcpy(	(void *)ph->p_va,
			(const void *)eh + ph->p_offset,
			ph->p_filesz);
		memset(	(void *)ph->p_va + ph->p_filesz, 
			0, 
			ph->p_memsz - ph->p_filesz);
	}

	p_proc->user_regs.eip = eh->e_entry;//此处将程序入口放到eip中
}

/*
 * 将用户栈映射到用户能够访问到的最后一个页面
 * (0xbffff000~0xc0000000)
 * 并将esp寄存器放置好
 */
void
map_stack(PROCESS_0 *p_proc)
{
	assert(p_proc->lock != 0);

	lin_mapping_phy(p_proc->cr3,
			&p_proc->page_list,
			K_PHY2LIN(-PGSIZE),
			(phyaddr_t)-1,
			PTE_P | PTE_W | PTE_U);
	
	p_proc->user_regs.esp = K_PHY2LIN(0);//栈顶0xc0000000
}

/*
 * 根据page_list回收所有的页面（包括回收页面节点）
 */
void
recycle_pages(struct page_node *page_list)
{
	for (struct page_node *prevp, *p = page_list ; p ;) {
		phy_free_4k(p->paddr);//页表单页释放
		prevp = p, p = p->nxt;
		kfree(prevp);//释放page_node结构体所占内存
	}
}
/*
 * 实现父进程->子进程的内存复制
 */
void
map_fork(PROCESS_0 *p_fa,PROCESS_0 *p_son)
{
	for(struct page_node* tp_node = p_fa->page_list;tp_node;tp_node = tp_node->nxt)
	{
		if(tp_node->laddr == -1)//只复制物理页,页目录/页表子进程有专属的一套
		{
			continue;
		}
		assert(tp_node->laddr != -1);
		//建立虚拟页和物理页的映射关系
		lin_mapping_phy(p_son->cr3,
			&p_son->page_list,
			tp_node->laddr,
			(phyaddr_t)-1,
			PTE_P | PTE_W | PTE_U);
		//先将物理地址通过K_PHY2LIN映射到内核空间的虚拟地址,由于先前已经加载过子进程的CR3,所以可直接memcpy到相应虚拟地址
		memcpy((void *)tp_node->laddr,(const void *)K_PHY2LIN(tp_node->paddr),PGSIZE);
	}
}