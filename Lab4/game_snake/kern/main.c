#include "assert.h"
#include "stdio.h"
#include "string.h"
#include "process.h"
#include "protect.h"
#include "time.h"
#include "type.h"
#include "trap.h"
#include "x86.h"
#include "game.h"

/*
 * 修改8253A
 */
void refresh_8253A()
{
	outb(TIMER_MODE, RATE_GENERATOR);	//修改模式控制寄存器
	outb(TIMER0,(u8)(TIMER_FREQ/HZ));	//修改counter0,值为1193182/1000
	outb(TIMER0,(u8)((TIMER_FREQ/HZ) >> 8));//注意强制类型转换的优先级高于移位
	//不能直接outw(TIMER0,(u16)(TIMER_FREQ/HZ))
}


/*
 * 三个测试函数，用户进程的执行流
 */
void startGame();

// void TestA()
// {
// 	int i = 0;
// 	while(1){
// 		// kprintf("\x1b[mA%d.",i++);
// 		kprintf("\x1b[mA.",i++);//将前景色恢复为白色
// 		for (int j = 0 ; j < 5e7 ; j++)
// 			;//do nothing
// 	}
// }

void TestB()
{
	int i = 0;
	while(1){
		 kprintf("\x1b[mB%d.",i++);
		//kprintf("\x1b[mB.",i++);//将前景色恢复为白色
		for (int j = 0 ; j < 5e7 ; j++)
			;//do nothing
	}
}

void TestC()
{
	int i = 0;
	while(1){
		 kprintf("\x1b[31mC%d.",i++);
		//kprintf("\x1b[31mC.",i++);//将前景色改为红色
		for (int j = 0 ; j < 5e7 ; j++)
			;//do nothing
	}
}

// 每个栈空间有12kb大
#define STACK_PREPROCESS	0x3000
#define STACK_TOTSIZE		STACK_PREPROCESS * PCB_SIZE
// 用户栈（直接在内核开一个临时数组充当）
char process_stack[STACK_TOTSIZE];
// 指向当前进程pcb的指针
PROCESS *p_proc_ready;
// pcb表
PROCESS	proc_table[PCB_SIZE];//PCB_SIZE代表有几个PCB项，也即有几个用户程序
// 指针数组
void (*entry[]) = {
	startGame,
	TestB,
	TestC,
};
char pcb_name[][16] = {
	"startGame",
	"TestB",
	"TestC",
};

/*
 * 内核的main函数
 * 用于初始化用户进程，然后将执行流交给用户进程
 */
void kernel_main()
{
	kprintf("---start kernel main---\n");

	PROCESS *p_proc = proc_table;//指向PCB表
	char *p_stack = process_stack;//指向用户栈

	//填写PCB表
	for (int i = 0 ; i < PCB_SIZE ; i++, p_proc++) {
		strcpy(p_proc->p_name, pcb_name[i]);
		p_proc->regs.cs = (SELECTOR_FLAT_C & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_USER;
		p_proc->regs.ds = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_USER;
		p_proc->regs.es = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_USER;
		p_proc->regs.fs = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_USER;
		p_proc->regs.ss = (SELECTOR_FLAT_RW & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_USER;
		p_proc->regs.gs = (SELECTOR_VIDEO & SA_RPL_MASK & SA_TI_MASK)
			| RPL_USER;
		
		p_proc->regs.eip = (u32)entry[i];//指向三个test的入口
		p_stack += STACK_PREPROCESS;//由高向低，所以先加
		p_proc->regs.esp = (u32)p_stack;//设置用户栈栈顶指针
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 *///0001_0010_0000_0010
	}

	p_proc_ready = proc_table;//当前PCB进程指针指向第一个用户态程序的PCB
	
	refresh_8253A();//修改8253A模式
	enable_irq(CLOCK_IRQ);//开启时钟中断
	enable_irq(KEYBOARD_IRQ);//开启键盘中断
	restart();//跳到atrap.asm
	assert(0);
}
