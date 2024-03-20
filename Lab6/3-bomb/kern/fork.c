#include <assert.h>
#include <errno.h>
#include <string.h>
#include <x86.h>


#include <kern/kmalloc.h>
#include <kern/fork.h>
#include <kern/pmap.h>
#include <kern/syscall.h>
#include <kern/sche.h>
#include <kern/trap.h>
#include <kern/stdio.h>

static u32 pid_num = 1;			//记录当前最大的pid
static u32 pid_num_lock = 0;	//最大pid锁
static u32 PCB_pre_use_lock = 0;//PCB排它锁

ssize_t
kern_fork(PROCESS_0 *p_fa)
{

	// fork的第一步你需要找一个空闲（IDLE）的进程作为你要fork的子进程
	PROCESS_0 *p_new_son;
	while(xchg(&p_fa->lock,1)==1)//先给父亲加锁
		schedule();
	int idle_pos = 0;
	while (xchg(&PCB_pre_use_lock, 1) == 1)	//给PCB_上派他锁，防止多个进程同时fork，读到脏数据
			schedule();
	//锁内进行PCB空闲项搜索
	for(idle_pos=0;idle_pos<PCB_SIZE;idle_pos++)
	{
		if(proc_table[idle_pos].pcb.statu == IDLE)	//此时发生调度，可能有wait将新的子进程设为IDLE，不过不影响，只要保证没有新的fork将其占用即可
		{
			p_new_son = &(proc_table[idle_pos].pcb);
			while(xchg(&p_new_son->lock,1)==1)//再给孩子加锁
				schedule();
			p_new_son->statu = INITING;		//设置为INITING,防止被调度
			break;
		}	
	}//如果此时被调度走，相关进程执行exit也无影响，因为exit只会将状态设为zombie而不是idle，还得等唯一的父进程调用wait才可变为idle
	if(idle_pos==PCB_SIZE)	//超出PCB限制的情况
	{
		xchg(&PCB_pre_use_lock, 0);
		xchg(&p_fa->lock,0);
		return -EAGAIN;
	}

	//给pid_num上锁
	while (xchg(&pid_num_lock, 1) == 1)	
		schedule();
	pid_num++;
	xchg(&pid_num_lock, 0);

	//用户栈复刻
	p_new_son->user_regs = p_fa->user_regs;
	//内核栈准备
	p_new_son->kern_regs.esp = (u32)(proc_table + idle_pos + 1) - 8;
	*(u32 *)(p_new_son->kern_regs.esp + 0) = (u32)restart;
	*(u32 *)(p_new_son->kern_regs.esp + 4) = (u32)p_new_son;

	p_new_son->priority = p_new_son->ticks = 1;	//时间片不一样
	p_new_son->pid = idle_pos;					//进程号和父进程不一样
	p_new_son->user_regs.eax = 0;				//子进程返回值为0

	//分页准备
	phyaddr_t ori_cr3 = phy_malloc_4k();
	memset((void *)K_PHY2LIN(ori_cr3), 0, PGSIZE);//对cr3页面进行清0
	struct page_node *ori_page_list = kmalloc(sizeof(struct page_node));
	ori_page_list->nxt = NULL;
	ori_page_list->paddr = ori_cr3;//页面列表的第一个结点是cr3页面
	ori_page_list->laddr = -1;	
	map_kern(ori_cr3, &ori_page_list);
	//加载页表
	DISABLE_INT();	//kern/trap.h
	p_new_son->cr3 = ori_cr3;
	p_new_son->page_list = ori_page_list;
	lcr3(p_new_son->cr3);
	map_fork(p_fa,p_new_son);
	lcr3(p_fa->cr3);		//加载回父进程页表
	ENABLE_INT();
	
	// 别忘了维护进程树，将这对父子进程关系添加进去
	p_new_son->fork_tree.p_fa = p_fa;//父结点
	p_new_son->fork_tree.sons = NULL;
	struct son_node *p_fa_son = p_fa->fork_tree.sons;
	struct son_node *p_proc_node = kmalloc(sizeof(struct son_node));
	p_proc_node->p_son = p_new_son;
	if(p_fa_son)//如果有孩子
	{
		assert(p_fa_son->pre == NULL);// 确保这个是双向链表头
		p_proc_node->nxt = p_fa_son;
		p_proc_node->pre = p_fa_son->pre;
		p_fa_son->pre = p_proc_node;
		p_fa->fork_tree.sons = p_proc_node;
	}
	else//如果没有孩子
	{
		p_fa->fork_tree.sons = p_proc_node;
		p_proc_node->nxt = NULL;
		p_proc_node->pre = NULL;
	}

	// 最后你需要将子进程的状态置为READY，说明fork已经好了，子进程准备就绪了
	p_new_son->statu = READY;

	xchg(&PCB_pre_use_lock, 0);
	xchg(&p_fa->lock, 0);
	xchg(&p_new_son->lock, 0);
	
	return p_new_son->pid;//返回子进程进程号
}

ssize_t
do_fork(void)
{
	return kern_fork(&p_proc_ready->pcb);
}
