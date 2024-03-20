#include <assert.h>
#include <string.h>
#include <syscall.h>
#include <mmu.h>
#include <errno.h>
#include <x86.h>
#include <kern/fork.h>
#include <kern/process.h>
#include <kern/sche.h>
#include <kern/trap.h>
#include <kern/pmap.h>
#include <kern/kmalloc.h>

//宏定义可以用于检查给定的地址x是否在内核空间范围内。如果x在内核空间中，则ISADDRINKERNEL(x)宏返回true，否则返回false。
#define ISADDRINKERNEL(x) (KERNBASE < (x) && (x) < KERNBASE + 128*MB)

int COPYFATHERPROCESS();
pcb_t* FIND_AN_IDLE();
void DESTROY_PROCESSTREE();


ssize_t
kern_fork(pcb_t *p_fa)
{
	// 这可能是你第一次实现一个比较完整的功能，你可能会比较畏惧
	// 但是放心，别怕，先别想自己要实现一个这么大的东西而毫无思路
	// 这样你在焦虑的同时也在浪费时间，就跟你在实验五中被页表折磨一样
	// 人在触碰到未知的时候总是害怕的，这是天性，所以请你先冷静下来
	// fork系统调用会一步步引导你写出来，不会让你本科造火箭的
	//panic("Unimplement! CALM DOWN!");

	// 推荐是边写边想，而不是想一车然后写，这样非常容易计划赶不上变化

	// fork的第一步你需要找一个空闲（IDLE）的进程作为你要fork的子进程
	//panic("Unimplement! find a idle process");
	int i,parent_id,child_id;
	proc_t *p_new = proc_table;
	while(xchg(&p_fa->lock,1) == 1)
		schedule();
	for(i = 0; i<PCB_SIZE; i++,p_new++)
	{
		if(p_new->pcb.statu == IDLE)
			break;
	}
	child_id = i;
	if(i == PCB_SIZE)
	{
		xchg(&p_fa->lock,0);
		return -EAGAIN;
	}
	assert(p_new == &proc_table[i]);

	parent_id = p_fa->pid;
	p_new->pcb.pid = child_id;
	DISABLE_INT();
	// 再之后你需要做的是好好阅读一下pcb的数据结构，搞明白结构体中每个成员的语义
	// 别光扫一遍，要搞明白这个成员到底在哪里被用到了，具体是怎么用的
	// 可能exec和exit系统调用的代码能够帮助你对pcb的理解，不先理解好pcb你fork是无从下手的
	//panic("Unimplement! read pcb");

	//设置新进程的内核栈，并在内核栈中存储相关的函数指针和数据，以便在进程切换时恢复执行。
	p_new->pcb.kern_regs.esp = (u32)(p_new +1) - 8;
	*(u32 *)(p_new->pcb.kern_regs.esp + 0) = (u32)restart;
	*(u32 *)(p_new->pcb.kern_regs.esp + 4) = (u32)p_new;
	// 在阅读完pcb之后终于可以开始fork工作了
	// 本质上相当于将父进程的pcb内容复制到子进程pcb中
	// 但是你需要想清楚，哪些应该复制到子进程，哪些不应该复制，哪些应该子进程自己初始化
	// 其中有三个难点
	// 1. 子进程"fork"的返回值怎么处理？（需要你对系统调用整个过程都掌握比较清楚，如果非常清晰这个问题不会很大）
	// 2. 子进程内存如何复制？（别傻乎乎地复制父进程的cr3，本质上相当于与父进程共享同一块内存，
	// 而共享内存肯定不符合fork的语义，这样一个进程写内存某块地方会影响到另一个进程，这个东西需要你自己思考如何复制父进程的内存）
	// 3. 在fork结束后，肯定会调度到子进程，那么你怎么保证子进程能够正常进入用户态？
	// （你肯定会觉得这个问题问的莫名其妙的，只能说你如果遇到那一块问题了就会体会到这个问题的重要性，
	// 这需要你对调度整个过程都掌握比较清楚）
	//panic("Unimplement! copy pcb?");
	COPYFATHERPROCESS(p_fa,&p_new->pcb);
	p_new->pcb.fork_tree.p_fa = p_fa;
	//将新的子进程添加到父进程的fork树中，以便维护进程之间的关系。如果已经存在子节点，则将新的子进程添加到末尾；
	if(p_fa->fork_tree.sons != NULL)
	{
		son_node_t *temp_node = p_fa->fork_tree.sons;
		son_node_t *new_node  = kmalloc(sizeof(son_node_t));
		new_node->p_son = & p_new->pcb;
		while(temp_node->nxt != NULL)
			temp_node = temp_node->nxt;
		while(xchg(&temp_node->p_son->lock,1) == 1)
			schedule();
		temp_node->nxt = new_node;
		new_node->pre = temp_node;
		new_node->nxt = NULL;
		xchg(&temp_node->p_son->lock,0);//上锁
	}
	//如果不存在子节点，则创建头节点，并将新的子进程作为唯一的子节点
	else
	{
		son_node_t *head_node  = kmalloc(sizeof(son_node_t));
		p_fa->fork_tree.sons =head_node;
		head_node->p_son = &p_new->pcb;
		head_node->nxt = NULL;
		head_node->p_son = NULL;
	}


	// 别忘了维护进程树，将这对父子进程关系添加进去
	//panic("Unimplement! maintain process tree");

	// 最后你需要将子进程的状态置为READY，说明fork已经好了，子进程准备就绪了
	//panic("Unimplement! change status to READY");
	p_new->pcb.statu = READY;

	xchg(&p_fa->lock,0);
	ENABLE_INT();
	return child_id;
	// 在你写完fork代码时先别急着运行跑，先要对自己来个灵魂拷问
	// 1. 上锁上了吗？所有临界情况都考虑到了吗？（永远要相信有各种奇奇怪怪的并发问题）
	// 2. 所有错误情况都判断到了吗？错误情况怎么处理？（RTFM->`man 2 fork`）
	// 3. 你写的代码真的符合fork语义吗？
	//panic("Unimplement! soul torture");

	pcb_t *child = FIND_AN_IDLE();  // 找到一个空闲进程作为子进程
	memcpy(&(child), p_fa, sizeof(pcb_t));  // 复制父进程的pcb内容到子进程
	COPYFATHERPROCESS(p_fa, child);  // 建立父子关系
	child->statu = READY;  // 将子进程的状态置为READY
	return child->pid;  // 返回子进程的pid
}

int COPYFATHERPROCESS(pcb_t *parent, pcb_t *child)
{
	// 将child进程添加到parent进程的子进程列表中
	// 这里假设进程拥有一个指向第一个子进程的指针child，以及一个指向下一个兄弟进程的指针sibling
	memcpy((void *)&child->user_regs, (void *)&parent->user_regs,sizeof(user_context_t));

	child->user_regs.eax = 0;
	child->priority = parent->priority;
	child->ticks = parent->ticks;

	child->cr3 = phy_malloc_4k();
	memset((void *)K_PHY2LIN(child->cr3),0,PGSIZE);
	page_node_t *new_page_list = kmalloc(sizeof(page_node_t));
	new_page_list->nxt = NULL;
	new_page_list->paddr = child->cr3;
	new_page_list->laddr = -1;
	map_kern(child->cr3,&new_page_list);

	for(page_node_t *parent_list = parent->page_list; parent_list; parent_list= parent_list->nxt)
	{
		if(parent_list->laddr == -1 || ISADDRINKERNEL(parent_list->laddr))
			continue;
		unsigned char temp[PGSIZE];
		lin_mapping_phy(child->cr3,&child->page_list,parent_list->laddr,-1,PTE_P | PTE_U | PTE_W);

		memcpy(temp,(void *)(parent_list->laddr),PGSIZE);
		lcr3(child->cr3);
		memcpy((void *)(parent_list->laddr),temp,PGSIZE);
		lcr3(parent->cr3);
	}

	return 0;


	if (parent->fork_tree.sons->p_son == NULL)
	{
		parent->fork_tree.sons->p_son = child;
	}
	else
	{
		pcb_t *temp = parent->fork_tree.sons->p_son;
		while (temp->fork_tree.sons->nxt->p_son != NULL)
		{
			temp = temp->fork_tree.sons->nxt->p_son;
		}
		temp->fork_tree.sons->nxt->p_son = child;
	}
}

void DESTROY_PROCESSTREE(pcb_t *new_process)
{
	// 销毁进程树的函数
	// 递归地遍历进程树，从叶子节点开始销毁进程
	// 并释放进程占用的资源
	if (new_process == NULL)
	{
		return;
	}
	DESTROY_PROCESSTREE(new_process->fork_tree.sons->p_son);  // 销毁子进程
	DESTROY_PROCESSTREE(new_process->fork_tree.sons->nxt->p_son);  // 销毁兄弟进程
	// 在这里释放进程占用的资源，例如栈空间等
	// 最后将进程的状态置为EXITED
	new_process->statu = ZOMBIE;
}

//找到一个空闲的进程，以便后续使用该进程作为子进程的副本
pcb_t* FIND_AN_IDLE(void)
{
	// 遍历进程表，找到一个空闲的进程
	for (int i = 0; i < PCB_SIZE; i++)
	{
		if (proc_table[i].pcb.statu == IDLE)
		{
			return &proc_table[i].pcb;
		}
	}
	return NULL;  // 如果没有找到空闲进程，返回NULL
}

ssize_t do_fork(void)
{
	return kern_fork(&p_proc_ready->pcb);
}

