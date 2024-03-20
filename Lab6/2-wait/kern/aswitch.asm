[BITS 32]

[section .text]

P_STACKBASE	equ	0
EFLAGSREG	equ	P_STACKBASE
EDIREG		equ	EFLAGSREG + 4
ESIREG		equ	EDIREG + 4
EBPREG		equ	ESIREG + 4
EBXREG		equ	EBPREG + 4
EDXREG		equ	EBXREG + 4
ECXREG		equ	EDXREG + 4
EAXREG		equ	ECXREG + 4
ESPREG		equ	EAXREG + 4


global switch_kern_context
;void	switch_kern_context(
;	struct kern_context *current_context,
;	struct kern_context *next_context
;)
; 程序语义在kern/process.c的schedule函数有讲

switch_kern_context:
	push	ebp				;
	mov	ebp, esp
	push	eax
	push	ebx
	mov	eax, [ebp + 8]
	mov	ebx, [ebp + 12]
	call	.inner_switch	
	pop	ebx
	pop	eax
	pop	ebp 
	ret
.inner_switch:
	;保存当前进程上下文
	mov	[eax + ESPREG], esp
	lea	esp, [eax + ESPREG]		;esp指向无用的kern_context中的esp位置
	push	eax
	push	ecx
	push	edx
	push	ebx
	push	ebp
	push	esi
	push	edi
	pushf

	;恢复新进程上下文
	mov	esp, ebx

	popf
	pop	edi
	pop	esi
	pop	ebp
	pop	ebx
	pop	edx
	pop	ecx
	pop	eax
	pop	esp	;切换到(u32)(p_proc_ready + 1) - 8;
	ret		;此时直接跳到restart