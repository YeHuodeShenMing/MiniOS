#include "swap.h"

struct SwapInfo swapinfo;

// 这里创建了2个函数get_pte_phaddr和get_page_phaddr，实际上和pagetbl.c里面get_pte_phy_addr和get_page_phy_addr作用是一样的，只不过将pid索引换成了直接输入物理地址
/*======================================================================*
			 get_pte_phaddr		// add by pjt & ljc
*======================================================================*/
// inline error 啊?
u32 get_pte_phaddr(u32 PageDirPhyAddr,			    //页目录物理地址		//edit by visual 2016.5.19
				u32 AddrLin)						    //线性地址
{											    //获取该线性地址所属页表的物理地址
	return (*((u32 *)K_PHY2LIN(PageDirPhyAddr) + get_pde_index(AddrLin))) & 0xFFFFF000; //先找到该进程页目录首地址，然后计算出该线性地址对应的页目录项，再访问,最后注意4k对齐
}

/*======================================================================*
			 get_page_phaddr	// add by pjt & ljc
*======================================================================*/
inline u32 get_page_phaddr(u32 PageTblPhyAddr,		     //页表物理地址				//edit by visual 2016.5.19
				u32 AddrLin)	     //线性地址
{							     //获取该线性地址对应的物理页物理地址
	return (*((u32 *)K_PHY2LIN(PageTblPhyAddr) + get_pte_index(AddrLin))) & 0xFFFFF000;
}

void swapinit()
{
	swapinfo.version = 0;	// 未用到
    swapinfo.pages = SWAP_PAGES;	// swap区能够容纳页的数量
    swapinfo.bitmap = K_PHY2LIN((void *)do_kmalloc(SWAP_BITMAP_BYTES));
    swapinfo.next_AvailIndex = 0;
    memset(swapinfo.bitmap, '\0', SWAP_BITMAP_BYTES);

    Global_Page.size = 0;
    Global_Page.head = NULL;
    Global_Page.tail = NULL;
	return;
}

/*根据pos计算出要修改的位置在地址addr中的索引，
然后根据target值分别进行位的清零或置位操作，并将修改后的值写回原地址addr中*/
inline static void setbit(void *addr, u32 pos, int v)
{
    u32 u = ((u32 *)addr)[pos / 32];
    if (v == 0)
    {
        u &= 0xFFFFFFFE << (pos % 32);
    }
    else
    {
        u |= 1 << (pos % 32);
    }
    ((u32 *)addr)[pos / 32] = u;
}

/*获取下一个可用的交换页面的索引*/
u32 getAvailIndex(struct SwapInfo *swap)
{
    if (swap->next_AvailIndex >= swap->pages)
    {
        swap->next_AvailIndex = 0;
    }

    u32 index = swap->next_AvailIndex;
    u32 len = swap->pages / 32;
    for (u32 i = 0; i < len; i++)
    {
        for (u32 bits = swap->bitmap[i], j = 0; j < 32; j++)
        {
            if (!(bits & 0x00000001))
            {
                index = i * 32 + j;
                return index;
            }
            bits >>= 1;
        }
    }
	return 0;
}

// 从内存将页换到swap交换区，这里本质上就是将内存这页的内容拷贝至swap，并修改位图
u32 swap_page_in(u32 vaddr)
{
	// 先通过getAvailIndex搜索下一个可用的索引，然后把要swap的物理页复制到buf，再用do_vwrite写入swap区
    u32 swap_idx = getAvailIndex(&swapinfo);

    int fdout = do_vopen(SWAP_FILE_NAME, O_RDWR);
    char buf[PAGE_SIZE];
    memset(buf, 0, PAGE_SIZE);
    memcpy(buf, (void *)vaddr, PAGE_SIZE);
    do_vlseek(fdout, swap_idx * PAGE_SIZE, SEEK_SET);
    do_vwrite(fdout, buf, PAGE_SIZE);
    real_close(fdout);
	setbit(swapinfo.bitmap, swap_idx, 1);
    return swap_idx;
}

// 从swap交换区换出来该页，看似是swap区对应内存不会再使用，认为全'\0'，实际上仅仅是改变了位图，使得这一页标记为可以重新使用
void swap_page_out(u32 swap_idx, u32 vaddr)
{
    // 从swap区该索引对应页将page读出来并拷贝到大小4096(1k)的buf中，并更改swap区对应的位图
    int fdout = do_vopen(SWAP_FILE_NAME, O_RDWR);
	char buf[PAGE_SIZE];
    do_vlseek(fdout, swap_idx * PAGE_SIZE, SEEK_SET);
    do_vread(fdout, buf, PAGE_SIZE);
    memcpy((void *)vaddr, buf, PAGE_SIZE);
    real_close(fdout);
    setbit(swapinfo.bitmap, swap_idx, 0);
}

void page_allocated_add(struct pgAllocatedInfo *temp)
{
	// 首个特殊
    if (Global_Page.size == 0)
    {
        temp->next = NULL;
        temp->pre = NULL;
        Global_Page.head = temp;
        Global_Page.tail = temp;
    }
    else
    {
		// 尾插法，并移动glo_page_allocated_4k.tail尾指针
        Global_Page.tail->next = temp;
        temp->pre = Global_Page.tail;
        Global_Page.tail = Global_Page.tail->next;
    }
    Global_Page.size++;
}

void page_allocated_remove(struct pgAllocatedInfo *temp)
{
	// 首个和最后一个特殊，这里也不需要特别指出只有一个的情况，因为只有一个那head就直接指向NULL
    if (Global_Page.head == temp)
    {
        Global_Page.head = temp->next; // temp.next!=NULL
        temp->next->pre = NULL;
    }
    else if (Global_Page.tail == temp)
    {
        Global_Page.tail = temp->pre;
        temp->pre->next = NULL;
	}
    // 中间的节点
    else
    {
        temp->pre->next = temp->next;
        temp->next->pre = temp->pre;
    }
    Global_Page.size--;
}

/* fifo换页算法，因为用的是尾插，直接取双向链表的队头
// pde_phaddr: cr3  page_directory
// 缺陷:   多进程可能会有错误，这个pd是当前进程的，如果当前进程以前并没有占用过内存，那么就会clear失败
*/
struct pgAllocatedInfo get_pgSwapOut(u32 pid)
{
    assert(Global_Page.head != NULL);
    assert(Global_Page.size > 0);
	// kprintf("111");
    struct pgAllocatedInfo *temp = Global_Page.head;
    for (int i = 0; i < Global_Page.size; i++)
    {
        if (pid == temp->pid)
		{
			page_allocated_remove(temp);
			return *(temp);
		} // 优先替换同一个进程的
        temp = temp->next;
    }
    // 找不到同进程合适的,就直接取第一个
    // 其实在我们这里 第1页是0号进程 第2页是1号进程
    // 所以如果在多进程中真的出现了这种情况，需要避免取到第1，2页
    assert(Global_Page.head->next->next != NULL);
	// 所以直接从第三页开始取
    temp = Global_Page.head->next->next; 
    page_allocated_remove(temp);
    return *(temp);
    
}
void clear_mem(u32 pde_phaddr, u32 pid)
{
    struct pgAllocatedInfo pgSwapOut = get_pgSwapOut(pid);

    u32 vaddr = pgSwapOut.vaddr & 0xFFFFF000;
    u32 paddr = pgSwapOut.paddr;
    u32 pte_phaddr = get_pte_phaddr(pde_phaddr, vaddr);

    PROCESS *pro = &proc_table[pgSwapOut.pid];
    PageList pgfault;
    for (LinkList p = (&(pro->task.memmap.text_pages))->next; p != &(pro->task.memmap.text_pages); p = p->next)
    {
        PageList page = getPageEntry(p);
        if (paddr == page->p_addr && page->flag == PG_ALLOCATED)
        {
            page->swap_index = swap_page_in(vaddr);
            page->flag = PG_SWAPOUT;
            page->p_addr = 0;
			// 一定是要最后才取消映射，不然前面取消了，会导致访问内存的时候page_fault
			write_page_pte(pte_phaddr, vaddr, 0, PG_USU | PG_RWW);
			do_free_4k(paddr);
			refresh_page_cache();
			return;
        }
    }
    for (LinkList p = (&(pro->task.memmap.data_pages))->next; p != &(pro->task.memmap.data_pages); p = p->next)
    {
        PageList page = getPageEntry(p);
        if (paddr == page->p_addr && page->flag == PG_ALLOCATED)
        {
            page->swap_index = swap_page_in(vaddr);
            page->flag = PG_SWAPOUT;
            page->p_addr = 0;
			// 一定是要最后才取消映射，不然前面取消了，会导致访问内存的时候page_fault
			write_page_pte(pte_phaddr, vaddr, 0, PG_USU | PG_RWW);
			do_free_4k(paddr);
			refresh_page_cache();
			return;
        }
    }
    for (LinkList p = (&(pro->task.memmap.stack_pages))->next; p != &(pro->task.memmap.stack_pages); p = p->next)
    {
        PageList page = getPageEntry(p);
        if (paddr == page->p_addr && page->flag == PG_ALLOCATED)
        {
            page->swap_index = swap_page_in(vaddr);
            page->flag = PG_SWAPOUT;
            page->p_addr = 0;
			// 一定是要最后才取消映射，不然前面取消了，会导致访问内存的时候page_fault
			write_page_pte(pte_phaddr, vaddr, 0, PG_USU | PG_RWW);
			do_free_4k(paddr);
			refresh_page_cache();
			return;
        }
    }
    panic(" [Error Segment]\n");
}

void swapout(PageList pglist, u32 pde_phaddr)
{
	int i = 0;
    u32 page_vaddr, page_paddr; // page的物理地址和虚拟地址
    u32 pte_phaddr;		

    page_vaddr = pglist->v_addr;
    pte_phaddr = get_pte_phaddr(pde_phaddr, page_vaddr);
    page_paddr = pglist->p_addr;
    pglist->swap_index = swap_page_in(page_vaddr);
    pglist->p_addr = 0;
	pglist->flag = PG_SWAPOUT;

    assert(Global_Page.head != NULL);
    assert(Global_Page.size > 0);
    struct pgAllocatedInfo *temp = Global_Page.head;
    for (; i < Global_Page.size; i++)
    {
        if (page_paddr == temp->paddr)
        {
            page_allocated_remove(temp);
			write_page_pte(pte_phaddr, page_vaddr,
						   0, PG_USU | PG_RWW);
			do_free_4k(page_paddr);
			refresh_page_cache();
			return;
        }
        temp = temp->next;
    }
}

void swapin(PageList pglist, u32 pde_phaddr)
{
    // check mem
    if (MIN_PAGE_COUNT <= Global_Page.size)
        clear_mem(p_proc_current->task.cr3, p_proc_current->task.pid);

    u32 page_vaddr = pglist->v_addr;
    u32 page_paddr = do_malloc_4k();
	u32 pte_phaddr = get_pte_phaddr(pde_phaddr, page_vaddr);
    assert(page_paddr != MAX_UNSIGNED_INT);
	pglist->p_addr = page_paddr;
    kprintf(" [swap_area allocate addr : %p ]\n", page_paddr);
    pglist->flag = PG_ALLOCATED;
    write_page_pte(pte_phaddr, page_vaddr, page_paddr,
				   PG_P | PG_USU | PG_RWW);
    swap_page_out(pglist->swap_index, page_vaddr);

    struct pgAllocatedInfo *temp = K_PHY2LIN((void *)do_kmalloc(sizeof(struct pgAllocatedInfo)));
    temp->paddr = page_paddr;
    temp->pid = p_proc_current->task.pid;
    temp->vaddr = page_vaddr & 0xFFFFF000;
	temp->segment = pglist->segment;
	temp->next = NULL;
    temp->pre = NULL;
    page_allocated_add(temp);
    refresh_page_cache();
}

