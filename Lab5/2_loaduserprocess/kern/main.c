#include <assert.h>
#include <mmu.h>
#include <string.h>
#include <type.h>
#include <x86.h>
#include <elf.h>

#include <kern/fs.h>
#include <kern/kmalloc.h>
#include <kern/stdio.h>
#include <kern/pmap.h>
#include <kern/process.h>
#include <kern/protect.h>
#include <kern/trap.h>


#define tt 1
// 标志着内核是否处理完成
bool init_kernel;

// 指向当前进程pcb的指针
proc_t *p_proc_ready;
// pcb表
proc_t	proc_table[PCB_SIZE];

static inline void
init_segment_regs(proc_t *p_proc)
{
	p_proc->pcb.user_regs.cs = (SELECTOR_FLAT_C & SA_RPL_MASK & SA_TI_MASK)
		| SA_TIL | RPL_USER;
	p_proc->pcb.user_regs.ds = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
		| SA_TIL | RPL_USER;
	p_proc->pcb.user_regs.es = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
		| SA_TIL | RPL_USER;
	p_proc->pcb.user_regs.fs = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
		| SA_TIL | RPL_USER;
	p_proc->pcb.user_regs.ss = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
		| SA_TIL | RPL_USER;
	p_proc->pcb.user_regs.gs = (SELECTOR_VIDEO & SA_RPL_MASK & SA_TI_MASK)
		| RPL_USER;
}

/*
 * 内核的main函数
 * 用于初始化用户进程，然后将执行流交给用户进程
 */
void kernel_main(void)
{
	kprintf("---start kernel main---\n");

	proc_t *p_proc = proc_table;
	char *p_stack = 0;
	for (int i = 0 ; i < PCB_SIZE ; i++, p_proc++) {
		// 初始化进程段寄存器
		init_segment_regs(p_proc);
		// 为进程分配cr3物理内存，cr3是页目录的首地址，也维护着进程的地址空间
		p_proc->pcb.cr3 = phy_malloc_4k();
		// 将3GB~3GB+128MB的线性地址映射到0~128MB的物理地址
		map_kern(p_proc->pcb.cr3);
		// 在map_kern之后，就内核程序对应的页表已经被映射了
		// 就可以直接lcr3，于此同时执行流不会触发page fault
		// 如果不先map_kern，执行流会发现执行的代码的线性地址不存在爆出Page Fault
		// 当然选不选择看个人的想法，评价是都行，各有各的优缺点
		lcr3(p_proc->pcb.cr3);
		static char filename[PCB_SIZE][12] = {
			"TESTKEY BIN",
			"TESTPID BIN",
		};
		// 从磁盘中将文件读出，需要注意的是要满足短目录项的文件名长度11，
		// 前八个为文件名，后三个为后缀名，跟BootLoader做法一致
		// 推荐将文件加载到3GB + 48MB处，应用程序保证不会有16MB那么大
		read_file(filename[i], (void *)K_PHY2LIN(48 * MB));
		// 现在你就需要将从磁盘中读出的ELF文件解析到用户进程的地址空间中
		u32 max_addr = 0,proc_addr;
		//panic("unimplement! load elf file");
		Elfhdr_t *elf = (void *)K_PHY2LIN(48 * MB);
		Proghdr_t *phd = (void *)elf + elf->e_phoff;
		for (u32 i = 0; i < elf->e_phnum; i++, phd++)
		{
			if (phd->p_type != PT_LOAD)
			 	continue;
			proc_addr = phd->p_va + phd->p_memsz - 1;
			if(proc_addr > max_addr)
			{
				max_addr = proc_addr;
			}
		}
		map_client(p_proc->pcb.cr3,max_addr);
		// 上一个实验中，我们开栈是用内核中的一个数组临时充当栈
		// 但是现在就不行了，用户是无法访问内核的地址空间（3GB ~ 3GB + 128MB）
		// 需要你自行处理给用户分配用户栈。
		//panic("allocate user stack");
		// 初始化用户寄存器
		phd = (void *)elf + elf->e_phoff;//重新回到程序头表头部
		
		for (u32 i = 0; i < elf->e_phnum; i++, phd++)
		{
			if (phd->p_type != PT_LOAD)
				continue;
			// 将elf的文件数据复制到指定位置
			memcpy(
			(void *)phd->p_va,	
			(void *)elf + phd->p_offset,
			phd->p_filesz);
			memset(
			(void *)phd->p_va + phd->p_filesz,
			0,
			phd->p_memsz - phd->p_filesz);
		}
		u32 user_stack_base = ((max_addr >> 12) << 12) + 4 * KB;	
		p_proc->pcb.user_regs.eflags = 0x1202; /* IF=1, IOPL=1 */
		p_proc->pcb.user_regs.eip = elf->e_entry;
		p_proc->pcb.user_regs.esp = user_stack_base + U_STACK_PREPROCESS;
		// 接下来初始化内核寄存器，
		// 为什么需要初始化内核寄存器原因是加入了系统调用后
		// 非常有可能出现系统调用执行过程中插入其余中断的情况，
		// 如果跟之前一样所有进程共享一个内核栈会发生不可想象的结果，
		// 为了避免这种情况，就需要给每个进程分配一个进程栈。
		// 当触发时钟中断发生调度的时候，不再是简单的切换p_proc_ready，
		// 而是需要将内核栈进行切换，而且需要切换执行流到另一个进程的内核栈。
		// 所以需要一个地方存放当前进程的寄存器上下文，这是一个富有技巧性的活，深入研究会觉得很妙，
		// 如果需要深入了解，去查看kern/process.c中的schedule函数了解切换细节。
		p_proc->pcb.kern_regs.esp = (u32)(p_proc + 1) - 8;
		// 保证切换内核栈后执行流进入的是restart函数。
		*(u32 *)(p_proc->pcb.kern_regs.esp + 0) = (u32)restart;
		// 这里是因为restart要用`pop esp`确认esp该往哪里跳。
		*(u32 *)(p_proc->pcb.kern_regs.esp + 4) = (u32)&p_proc->pcb;

		// 初始化其余量
		p_proc->pcb.pid = i;
		static int priority_table[PCB_SIZE] = {1,2};//三个delay进程的优先级相同
		// priority 预计给每个进程分配的时间片
		// ticks 进程剩余的进程片
		p_proc->pcb.priority = p_proc->pcb.ticks = priority_table[i];
	}

	p_proc_ready = proc_table;
	// 切换进程页表和tss
	lcr3(p_proc_ready->pcb.cr3);
	tss.esp0 = (u32)(&p_proc_ready->pcb.user_regs + 1);
	
	
	// 开个无用的kern_context存当前执行流的寄存器上下文（之后就没用了，直接放在临时变量中）
	struct kern_context idle;
	switch_kern_context(&idle, &p_proc_ready->pcb.kern_regs);
	assert(0);
} 

void
map_client(phyaddr_t cr3,u32 max_page_addr)
{
	u32 PDX_num = PDX(0x8048000),PTX_num = PTX(0x8048000);//PDX_num:虚拟地址高10位，PTX_num:虚拟地址的中间10位
	u32 max_ptx =  PTX(max_page_addr);
	uintptr_t *pde_ptr = (uintptr_t *)K_PHY2LIN(cr3);
	pde_ptr += PDX_num;
	phyaddr_t pte_phy = phy_malloc_4k();
	// 保证申请出来的物理地址是4k对齐的
	assert(PGOFF(pte_phy) == 0);
	*pde_ptr = pte_phy | PTE_P | PTE_W | PTE_U;
	uintptr_t *pte_ptr = (uintptr_t *)K_PHY2LIN(pte_phy);
	pte_ptr += PTX_num;
	for(u32 i = 0;i <= max_ptx;i++)
	{
		pte_phy = phy_malloc_4k();
		assert(PGOFF(pte_phy) == 0);
		*(pte_ptr++) = pte_phy | PTE_P | PTE_W | PTE_U;
	}
	//再多分配U_STACK_PREPROCESS的空间用作用户栈
	for(u32 i = 1;i <= U_STACK_PREPROCESS / (4 * KB);i++ )
	{
		pte_phy = phy_malloc_4k();
		assert(PGOFF(pte_phy) == 0);
		*(pte_ptr++) = pte_phy | PTE_P | PTE_W | PTE_U;
	}	
	return;
}
