[section .text]

; 导入
extern	StackTop		; kern/astart.asm
extern	tss			; inc/kern/protect.h
extern	exception_handler	; inc/kern/trap.h
extern	irq_table		; inc/kern/trap.h
extern	syscall_handler		; inc/kern/trap.h
extern	p_proc_ready		; inc/kern/process.h

; 具体看inc/kern/process.h pcb维护的寄存器表
P_STACKBASE	equ	0
GSREG		equ	P_STACKBASE
FSREG		equ	GSREG		+ 4
ESREG		equ	FSREG		+ 4
DSREG		equ	ESREG		+ 4
EDIREG		equ	DSREG		+ 4
ESIREG		equ	EDIREG		+ 4
EBPREG		equ	ESIREG		+ 4
KERNELESPREG	equ	EBPREG		+ 4
EBXREG		equ	KERNELESPREG	+ 4
EDXREG		equ	EBXREG		+ 4
ECXREG		equ	EDXREG		+ 4
EAXREG		equ	ECXREG		+ 4
RETADR		equ	EAXREG		+ 4
EIPREG		equ	RETADR		+ 4
CSREG		equ	EIPREG		+ 4
EFLAGSREG	equ	CSREG		+ 4
ESPREG		equ	EFLAGSREG	+ 4
SSREG		equ	ESPREG		+ 4
P_STACKTOP	equ	SSREG		+ 4

extern	to_kern_stack	; kern/process.c

save:
        pushad          ; `.
        push	ds      ;  |
        push	es      ;  | 保存原寄存器值
        push	fs      ;  |
        push	gs      ; /
        mov	dx, ss
        mov	ds, dx
        mov	es, dx

        mov	eax, esp		; 保存完寄存器后esp已到pcb用户寄存器底部，用户寄存器已压满
	test	dword [eax + CSREG - P_STACKBASE], 3
        jz	.1			; 根据段寄存器的状态信息判断是否发生内核重入
        mov	esp, StackTop		; 如果没有发生内核重入就意味着要到用户栈了，先将esp移入临时内核栈
	push	eax			; 传入进程表首地址信息
	call	to_kern_stack		; 这个函数之后esp变为进程内核栈
	pop	eax			; 恢复进程表首地址信息（这个是restart第一句话要用的）
.1:
	push	eax			; 将restart要用的，如果在内核栈重入，pop esp不会有任何变化
        push	restart			; 否则eax在进程表首地址，pop esp会使esp移动到用户栈栈底
        jmp	[eax + RETADR - P_STACKBASE]

restart:				; 先把保存在用户内核栈的PCB底部地址弹到esp里
	pop	esp			; 获悉当前的esp该到哪，不用管现在是否要回用户态，语义在save中有解释
	pop	gs
	pop	fs
	pop	es
	pop	ds
	popad			;此时也恢复了eip，执行流转换
	add	esp, 4
	iretd			;iretd把用户栈的esp也给恢复

; 系统调用
int_syscall:
	call	save
	sti	
	call	syscall_handler
	cli
	ret

EOI		equ	0x20
INT_M_CTL	equ	0x20	; I/O port for interrupt controller         <Master>
INT_M_CTLMASK	equ	0x21	; setting bits in this port disables ints   <Master>
INT_S_CTL	equ	0xA0	; I/O port for second interrupt controller  <Slave>
INT_S_CTLMASK	equ	0xA1	; setting bits in this port disables ints   <Slave>

; 中断和异常 -- 硬件中断
; ---------------------------------
%macro	hwint_master	1
	call	save
	in	al, INT_M_CTLMASK	; `.
	or	al, (1 << %1)		;  | 屏蔽当前中断
	out	INT_M_CTLMASK, al	; /
	mov	al, EOI			; `. 置EOI位
	out	INT_M_CTL, al		; /
	sti	; CPU在响应中断的过程中会自动关中断，这句之后就允许响应新的中断
	push	%1			; `.
	call	[irq_table + 4 * %1]	;  | 中断处理程序
	pop	ecx			; /将中断号pop到ecx里，现在剩restart--pcb底部
	cli
	in	al, INT_M_CTLMASK	; `.
	and	al, ~(1 << %1)		;  | 恢复接受当前中断
	out	INT_M_CTLMASK, al	; /
	ret
%endmacro

; 时钟中断采取全程关中断的模式，时钟中断相当重要，不允许被打扰
ALIGN	16
hwint00:		; Interrupt routine for irq 0 (the clock).
	call	save
	mov	al, EOI
	out	INT_M_CTL, al

	push	0
	call	[irq_table + 0]
	pop	ecx

	ret
	
ALIGN	16
hwint01:		; Interrupt routine for irq 1 (keyboard)
	hwint_master	1

ALIGN	16
hwint02:		; Interrupt routine for irq 2 (cascade!)
	hwint_master	2

ALIGN	16
hwint03:		; Interrupt routine for irq 3 (second serial)
	hwint_master	3

ALIGN	16
hwint04:		; Interrupt routine for irq 4 (first serial)
	hwint_master	4

ALIGN	16
hwint05:		; Interrupt routine for irq 5 (XT winchester)
	hwint_master	5

ALIGN	16
hwint06:		; Interrupt routine for irq 6 (floppy)
	hwint_master	6

ALIGN	16
hwint07:		; Interrupt routine for irq 7 (printer)
	hwint_master	7

; ---------------------------------
%macro	hwint_slave	1
	hlt		; 后面的8个外设中断暂时不需要，先hlt休眠核
%endmacro
; ---------------------------------

ALIGN	16
hwint08:		; Interrupt routine for irq 8 (realtime clock).
	hwint_slave	8

ALIGN	16
hwint09:		; Interrupt routine for irq 9 (irq 2 redirected)
	hwint_slave	9

ALIGN	16
hwint10:		; Interrupt routine for irq 10
	hwint_slave	10

ALIGN	16
hwint11:		; Interrupt routine for irq 11
	hwint_slave	11

ALIGN	16
hwint12:		; Interrupt routine for irq 12
	hwint_slave	12

ALIGN	16
hwint13:		; Interrupt routine for irq 13 (FPU exception)
	hwint_slave	13

ALIGN	16
hwint14:		; Interrupt routine for irq 14 (AT winchester)
	hwint_slave	14

ALIGN	16
hwint15:		; Interrupt routine for irq 15
	hwint_slave	15

; 中断和异常 -- 异常
divide_error:
	push	0xFFFFFFFF	; no err code
	push	0		; vector_no	= 0
	jmp	exception
single_step_exception:
	push	0xFFFFFFFF	; no err code
	push	1		; vector_no	= 1
	jmp	exception
nmi:
	push	0xFFFFFFFF	; no err code
	push	2		; vector_no	= 2
	jmp	exception
breakpoint_exception:
	push	0xFFFFFFFF	; no err code
	push	3		; vector_no	= 3
	jmp	exception
overflow:
	push	0xFFFFFFFF	; no err code
	push	4		; vector_no	= 4
	jmp	exception
bounds_check:
	push	0xFFFFFFFF	; no err code
	push	5		; vector_no	= 5
	jmp	exception
inval_opcode:
	push	0xFFFFFFFF	; no err code
	push	6		; vector_no	= 6
	jmp	exception
copr_not_available:
	push	0xFFFFFFFF	; no err code
	push	7		; vector_no	= 7
	jmp	exception
double_fault:
	push	8		; vector_no	= 8
	jmp	exception
copr_seg_overrun:
	push	0xFFFFFFFF	; no err code
	push	9		; vector_no	= 9
	jmp	exception
inval_tss:
	push	10		; vector_no	= A
	jmp	exception
segment_not_present:
	push	11		; vector_no	= B
	jmp	exception
stack_exception:
	push	12		; vector_no	= C
	jmp	exception
general_protection:
	push	13		; vector_no	= D
	jmp	exception
page_fault:
	push	eax
	mov	eax, [esp + 4]
	mov	[StackTop - 1024], eax
	pop	eax
	pushad          ; `.
        push	ds      ;  |
        push	es      ;  | 保存原寄存器值
        push	fs      ;  |
        push	gs      ; /
        mov	dx, ss
        mov	ds, dx
        mov	es, dx

        mov	eax, esp
	test	dword [eax + CSREG - P_STACKBASE], 3
        jz	.1			; 根据段寄存器的状态信息判断是否发生内核重入
        mov	esp, StackTop		; 如果没有发生内核重入就意味着要到用户栈了，先将esp移入临时内核栈
	push	eax			; 传入进程表首地址信息
	call	to_kern_stack		; 这个函数之后esp变为进程内核栈
	pop	eax			; 恢复进程表首地址信息（这个是restart第一句话要用的）
.1:
	mov	ecx, [eax + EFLAGSREG - P_STACKBASE]
	push	ecx
	mov	ecx, [eax + CSREG - P_STACKBASE]
	push	ecx
	mov	ecx, [eax + EIPREG - P_STACKBASE]
	push	ecx
	mov	ebx, [StackTop - 1024]
	push	ebx	
	push	14		; vector_no	= E
	jmp	exception
copr_error:
	push	0xFFFFFFFF	; no err code
	push	16		; vector_no	= 10h
	jmp	exception

exception:
	call	exception_handler
	add	esp, 4*2	; 让栈顶指向 EIP，堆栈中从顶向下依次是：EIP、CS、EFLAGS
	hlt

; 一堆符号导出，没别的
global restart
; 系统调用
global int_syscall
; 异常处理
global	divide_error
global	single_step_exception
global	nmi
global	breakpoint_exception
global	overflow
global	bounds_check
global	inval_opcode
global	copr_not_available
global	double_fault
global	copr_seg_overrun
global	inval_tss
global	segment_not_present
global	stack_exception
global	general_protection
global	page_fault
global	copr_error
; 外设中断
global	hwint00
global	hwint01
global	hwint02
global	hwint03
global	hwint04
global	hwint05
global	hwint06
global	hwint07
global	hwint08
global	hwint09
global	hwint10
global	hwint11
global	hwint12
global	hwint13
global	hwint14
global	hwint15