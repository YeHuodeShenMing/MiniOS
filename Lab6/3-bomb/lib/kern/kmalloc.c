#include <assert.h>
#include <mmu.h>
#include <x86.h>

#include <kern/stdio.h>
#include <kern/kmalloc.h>
#include <kern/sche.h>
#include <kern/trap.h>
#include <kern/pmap.h>
static u32 phy_malloc_4k_lock;
static phyaddr_t phy_malloc_4k_p = 96 * MB;

/*
 * 释放物理页面，这里并没有为你实现好free_4k的代码，
 * 你需要自己实现一个数据结构用于维护释放的页面，
 * 需要有回收资源的意识，这是一个系统必须有的功能。
 */
static struct page_node* free_page_list=NULL;//回收页面链表
static u32 phy_free_4k_lock=0;
void
phy_free_4k(phyaddr_t paddr)
{
	while(xchg(&phy_free_4k_lock, 1) == 1)//上锁
		schedule();
	assert(paddr % PGSIZE == 0);
	struct page_node* new_free_node = kmalloc(sizeof(struct page_node));
	new_free_node->paddr = paddr;	//头插法
	new_free_node->nxt = free_page_list;
	new_free_node->laddr = -1;
	free_page_list = new_free_node;
	xchg(&phy_free_4k_lock, 0);//释放锁
}
/*
 * 分配物理页面，每次分配4kb，一页
 * 分配的物理内存区间为96MB~128MB
 */
phyaddr_t
phy_malloc_4k(void)
{
	while(xchg(&phy_malloc_4k_lock, 1) == 1)//线性malloc上锁
		schedule();
	while(xchg(&phy_free_4k_lock, 1) == 1)	//回收页链表上锁
		schedule();
	phyaddr_t paddr;
	if(free_page_list)	//如果回收页链表里有页,就从该链表里取
	{
		struct page_node* h_fpage = free_page_list;
		paddr = free_page_list->paddr;
		free_page_list = free_page_list->nxt;
		kfree((void *)h_fpage);
	}
	else				//如果没有,则接着按线性接着向后分配
	{
		assert(phy_malloc_4k_p < 128 * MB);
		paddr = phy_malloc_4k_p;
		phy_malloc_4k_p += PGSIZE;
	}
free:
	xchg(&phy_malloc_4k_lock, 0);
	xchg(&phy_free_4k_lock, 0);
	return paddr;
}
//
struct kmalloc_header {
	struct kmalloc_header *ptr;
	size_t size;
};
//释放的内存指针
static struct kmalloc_header *freePtr;
static u32 malloc_lock;

/*
 * 释放kmalloc申请的内存
 */
void
kfree(void *v)
{
	struct kmalloc_header *bp, *p;
	bp = (struct kmalloc_header*)v - 1;

	while(xchg(&malloc_lock, 1) == 1)
		schedule();

	for (p = freePtr; !(bp > p && bp < p->ptr); p = p->ptr) {
		if (p >= p->ptr && (bp > p || bp < p->ptr)) {
			break;
		}
	}
	if (bp + bp->size == p->ptr) {
		bp->size += p->ptr->size;
		bp->ptr = p->ptr->ptr;
	} else {
		bp->ptr = p->ptr;
	}
	if (p + p->size == bp) {
		p->size += bp->size;
		p->ptr = bp->ptr;
	} else {
		p->ptr = bp;
	}
	freePtr = p;
free:
	xchg(&malloc_lock, 0);
}

/*
 * 分配内存，大小为n字节。
 * 分配物理内存区间为64MB~96MB
 */
void *
kmalloc(size_t n)
{
	struct kmalloc_header *p, *prevP;
	size_t nUnits;
	void *ret;

	while(xchg(&malloc_lock, 1) == 1)
		schedule();

	nUnits = (n + sizeof(struct kmalloc_header) - 1) 
			/ sizeof(struct kmalloc_header) + 1;
	if ((prevP = freePtr) == 0) {
		freePtr = prevP = (void *)K_PHY2LIN(64 * MB);
		freePtr->ptr = freePtr;
		freePtr->size = (32 * MB) / sizeof(struct kmalloc_header);
	}
	for (p = prevP->ptr ; ; prevP = p, p = p->ptr) {
		if (p->size >= nUnits) {
			if (p->size == nUnits) {
				prevP->ptr = p->ptr;
			} else {
				p->size -= nUnits;
				p += p->size;
				p->size = nUnits;
			}
			freePtr = prevP;
			ret = (void*)(p + 1);
			goto free;
		}
		if (p == freePtr) {
			panic("malloc failed!");
		}
	}
free:
	xchg(&malloc_lock, 0);
	return ret;
}
