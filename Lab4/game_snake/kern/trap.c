#include "assert.h"
#include "process.h"
#include "keyboard.h"
#include "stdio.h"
#include "time.h"
#include "trap.h"
#include "x86.h"
#include "keymap.h"
/*
 * 当前内核需要处理中断的数量
 */
int k_reenter;

void (*irq_table[16])(int) = {
	clock_interrupt_handler,
	keyboard_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
	default_interrupt_handler,
};

/*
 * 开启对应外设中断（将掩码对应位置为0）
 */
void
enable_irq(int irq)
{
	u8 mask = 1 << (irq % 8);
	if (irq < 8)
		outb(INT_M_CTLMASK, inb(INT_M_CTLMASK) & ~mask);
	else
		outb(INT_S_CTLMASK, inb(INT_S_CTLMASK) & ~mask);
}

/*
 * 关闭对应外设中断（将掩码对应位置为1）
 */
void
disable_irq(int irq)
{
	u8 mask = 1 << (irq % 8);
	if (irq < 8)
		outb(INT_M_CTLMASK, inb(INT_M_CTLMASK) | mask);
	else
		outb(INT_S_CTLMASK, inb(INT_S_CTLMASK) | mask);
}
/*
 * 中断默认处理函数
 * 理论上是不支持的，所以会给个warning
 */
void
default_interrupt_handler(int irq)
{
	warn("unsupport interrupt! irq = %d", irq);
}

/*
 * 异常默认处理函数
 * 由于没有任何处理异常的手段，所以会给个panic
 */
void
exception_handler(int vec_no, int err_code, int eip, int cs, int eflags)
{
	char err_description[][64] = {	"#DE Divide Error",
					"#DB RESERVED",
					"—  NMI Interrupt",
					"#BP Breakpoint",
					"#OF Overflow",
					"#BR BOUND Range Exceeded",
					"#UD Invalid Opcode (Undefined Opcode)",
					"#NM Device Not Available (No Math Coprocessor)",
					"#DF Double Fault",
					"    Coprocessor Segment Overrun (reserved)",
					"#TS Invalid TSS",
					"#NP Segment Not Present",
					"#SS Stack-Segment Fault",
					"#GP General Protection",
					"#PF Page Fault",
					"—  (Intel reserved. Do not use.)",
					"#MF x87 FPU Floating-Point Error (Math Fault)",
					"#AC Alignment Check",
					"#MC Machine Check",
					"#XF SIMD Floating-Point Exception"
				};
	panic("\x1b[H\x1b[2JException! --> %s\nEFLAGS: %x CS: %x EIP: %x\nError code: %x",
		err_description[vec_no], eflags, cs, eip, err_code);
}
/*
 * 时钟中断处理函数
 */

void
clock_interrupt_handler(int irq)
{
	timecounter_inc();
}
void keyboard_interrupt_handler(int irq)
{
	u8 getkey = inb(KEYBOARD_PORT);//获取扫描码
	u8 ascii_code;
	if((getkey >= 0X10 && getkey <= 0X19) ||
		(getkey >= 0X1E && getkey <= 0x26) ||
		(getkey >= 0x2C && getkey <= 0x32))//扫描码为a-z时
	{
		ascii_code = (u8)keymap[getkey];//映射成ascii码
		add_keyboard_buf(ascii_code);//将字符添加到内核缓冲区
		// kprintf("%c",getch());//直接在屏幕上输出刚放入内核缓冲区的字符，以作检验
		
	}
	
}
// void
// clock_interrupt_handler(int irq)
// {
// 	// kprintf("i%d",clock());//时间戳的值就是当前时钟中断的次数
// 	timecounter_inc();
// 	if(clock()%20 < 2)	//在小的时间段内选择testc进程
// 	{
// 		p_proc_ready = proc_table + 2; //让进程testc饥饿
// 	}
// 	else				//其他情况，时间戳为偶数执行testA，时间戳为奇数执行testB
// 	{
// 		p_proc_ready = proc_table + clock()%2;
// 	}
// 	// p_proc_ready++;
// 	// if (p_proc_ready >= proc_table + PCB_SIZE) {
// 	// 	p_proc_ready = proc_table;//循环
// 	// }
// }
	
	



