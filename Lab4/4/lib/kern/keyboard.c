#include <stdio.h>
#include <type.h>

#include <kern/keymap.h>

#define KB_INBUF_SIZE 4

struct s_kb_inbuf {
	u8*	p_head;
	u8*	p_tail;
	int	count;
	u8	buf[KB_INBUF_SIZE];
};
typedef struct s_kb_inbuf kb_inbuf_t;

static kb_inbuf_t kb_input = {
	.p_head = kb_input.buf,
	.p_tail = kb_input.buf,
	.count = 0,
};

void
add_keyboard_buf(u8 ch)
{
	if(kb_input.count < KB_INBUF_SIZE)	//当缓冲区没有满
	{
		*(kb_input.p_tail++)= ch;	//添加字符
		kb_input.count++;
		if(kb_input.p_tail == kb_input.buf + KB_INBUF_SIZE)//循环队列
			kb_input.p_tail = kb_input.buf;
	}
}

/*
 * 如果内核的字符缓冲区为空，则返回-1
 * 否则返回缓冲区队头的字符并弹出队头
 */
u8
getch(void)
{
	u8 res;
	if(!kb_input.count)//当缓冲区为空
		return -1;
	else
	{
		// disable_int();//关中断
		res = *(kb_input.p_head++);//弹出字符
		if(kb_input.p_head == kb_input.buf + KB_INBUF_SIZE)//循环队列
			kb_input.p_head = kb_input.buf;
		kb_input.count--;
		// enable_int();//开中断
		return res;
	}
}
