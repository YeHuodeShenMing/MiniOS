[SECTION .text]

[BITS 32]

global kprintf
;===============================================
; void kprintf(u16 disp_pos, const char *format, ...)
; 参数说明：
; disp_pos: 开始打印的位置，0为0行0列，1为0行1列，80位1行0列
; format: 需要格式化输出的字符串，默认输出的字符颜色为黑底白字
; %c: 输出下一个参数的字符信息（保证参数范围在0~127），输出完打印的位置往下移动一位
; %b: 更改之后输出的字符的背景色（保证参数范围在0~15）
; %f: 更改之后输出的字符的前景色（保证参数范围在0~15）
; 输出是独立的，输出完打印的位置不会往下移动一位，不会影响接下来%c的输出的颜色
; 其余字符：按照字符输出（保证字符里不会有%，\n等奇奇怪怪的字符，都是常见字符，%后面必会跟上述三个参数之一），输出完打印的位置往下移动一位

ArgAddrOffset	dw	16		;下一个参数相对与ebp的地址偏移，初值是第三个参数的偏移量，为16
DispColor	db	0x0e

kprintf:
	jmp $
	push	ebp
	mov	ebp, esp
	pusha
	mov	eax, [ebp+8]		;打印的起始位置
	mov	esi, [ebp+12]		;字符串起始地址，这两个参数都是必定存在的，直接用ebp+常量寻址
	shl	eax, 1			;eax*2，用于显示字符
	mov	ebx, eax		;ebx<-eax
	mov	ecx, 16
	mov	[ArgAddrOffset], cx	;初始化变量
	mov	ah, 0fh		;默认为黑底白字
	mov	ecx, 0			;ecx清零

	push	ebx			;保存显示位置
.loop:
	mov     bl, [esi+ecx]		;从字符串中加载一个字符
	cmp	bl, '%'		
	jne	.PrintChar		;不是%就直接打印
	inc	ecx			
	mov	bl, [esi+ecx]		
	cmp	bl, 'f'
	je	.loop_case_f
	cmp	bl, 'b'
	je	.loop_case_b
	cmp	bl, 'c'
	je	.loop_case_c
	
.loop_case_f:				;%f，改变字符颜色（前景色）
	and	ah, 0xf0		;ah低4位清零
	push	ax			;保存ax
	mov	eax, 0			;清零eax
	mov	ax, word [ArgAddrOffset] ;ax <- 当前需要加载参数相对于ebp的偏移量
	add 	word [ArgAddrOffset], 4 ;下一个参数相对于ebp的偏移量
	mov	edx, [ebp+eax]		;加载参数
	pop	ax			;恢复ax
	add	ah, dl			;更新前景色
	inc	ecx			
	jmp	.loop
	
.loop_case_b:				;%b，改变背景色
	and	ah, 0x0f		;ah高四位清零
	push	ax			;保存ax
	mov	eax, 0			;清零eax
	mov	ax, word[ArgAddrOffset] ;ax <- 当前需要加载参数相对于ebp的偏移量
	add	word [ArgAddrOffset], 4 ;下一个参数相对于ebp的偏移量
	mov	edx, [ebp+eax]		;加载参数
	pop	ax			;恢复ax
	shl	dl, 4			;dl左移4位
	add	ah, dl			;更新背景色
	inc	ecx			
	jmp	.loop
	
.loop_case_c:				;%c，将下一个参数作为字符输出
	push	ax			;保存ax
	mov	eax, 0			;清理eax
	mov	ax, word [ArgAddrOffset] ;ax <- 当前需要加载参数相对于ebp的偏移量
	add	word [ArgAddrOffset], 4 ;下一个参数相对于ebp的偏移量
	mov	edx, [ebp+eax]		;加载参数
	mov	bl, dl			;得到要输出字符的ascii码
	pop	ax			;恢复ax
					;直接到.PrintChar里执行
.PrintChar:
					;打印bl中所存以ascii码形式存储的字符
	mov	al, bl
	pop	ebx			;获得显示字符的位置
	cmp	al, 0			;如果是'\0'，代表结束
	je	.PrintEnd		
	mov 	word [gs:bx], ax	;通过显存显示字符
	add	bx, 2			;打印位置移动到下一位
	push	ebx			;保存下一个要打印的位置
	inc	ecx			
	jmp	.loop			;继续循环
	
.PrintEnd:
	popa
	mov	esp, ebp
	pop	ebp
	ret
	


