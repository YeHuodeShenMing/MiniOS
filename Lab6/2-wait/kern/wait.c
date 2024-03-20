#include <assert.h>
#include <mmu.h>
#include <errno.h>
#include <x86.h>

#include <kern/syscall.h>
#include <kern/wait.h>
#include <kern/pmap.h>
#include <kern/sche.h>
#include <kern/kmalloc.h>
#include <kern/trap.h>
ssize_t
kern_wait(int *wstatus)
{
_start_:
	while (xchg(&p_proc_ready->pcb.lock, 1) == 1)	//给父进程上锁,假如fork正在创建子进程,就需要提前上锁来保证wait看到的是对的
		schedule();	

	struct son_node* p_son_recy =p_proc_ready->pcb.fork_tree.sons;
	if(p_son_recy == NULL)//没有子进程,
	{
		xchg(&p_proc_ready->pcb.lock, 0);
		return -ECHILD;
	}
	for(;p_son_recy;p_son_recy = p_son_recy->nxt)
	{
		if(p_son_recy->p_son->statu == ZOMBIE)
		{
			while(xchg(&p_son_recy->p_son->lock, 1) == 1)
				schedule();
			break;
		}
	}
	
	if(p_son_recy == NULL)//没有可等待的孩子
	{
		DISABLE_INT();
		p_proc_ready->pcb.statu = SLEEP;
		xchg(&p_proc_ready->pcb.lock, 0);
		ENABLE_INT();
		schedule();
		goto _start_;		//重新对子进程结点进行遍历
	}

	//修改进程树
	if(p_son_recy->pre)//不是头节点
	{
		p_son_recy->pre->nxt = p_son_recy->nxt;
	}
	else	//当为头节点时,更改tree_node的sons指针
	{
		p_proc_ready->pcb.fork_tree.sons = p_son_recy->nxt;
	}
	if(p_son_recy->nxt)//不是尾节点
	{
		p_son_recy->nxt->pre = p_son_recy->pre;
	}
	if(wstatus)//如果不为null
		*wstatus = p_son_recy->p_son->exit_code;
	//回收页面资源
	recycle_pages(p_son_recy->p_son->page_list);
	//回收PCB资源
	p_son_recy->p_son->statu = IDLE;

	xchg(&p_proc_ready->pcb.lock, 0);
	xchg(&p_son_recy->p_son->lock, 0);
	
	PROCESS_0* p_cur_pcb = p_son_recy->p_son;
	kfree(p_son_recy);//释放子结点
	return p_cur_pcb->pid;
}

ssize_t
do_wait(int *wstatus)
{
	assert((uintptr_t)wstatus < KERNBASE);
	assert((uintptr_t)wstatus + sizeof(wstatus) < KERNBASE);
	return kern_wait(wstatus);
}


