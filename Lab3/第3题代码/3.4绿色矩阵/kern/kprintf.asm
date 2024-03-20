[SECTION .text]

[BITS 32]

global kprintf
;===============================================
; void kprintf(u16 disp_pos, const char *format, ...)
; 参数说明：
; disp_pos: 开始打印的位置，0为0行0列，1为0行1列，80位1行0列
; format: 需要格式化输出的字符串，默认输出的字符颜色为黑底白字
; %c: 输出下一个参数的字符信息（保证参数范围在0~127），输出完打印的位置往下移动一位
; %b: 更改下一个输出的字符的背景色（保证参数范围在0~15）
; %f: 更改下一个输出的字符的前景色（保证参数范围在0~15）
; 其余字符：按照字符输出（保证字符里不会有%，\n等奇奇怪怪的字符，都是常见字符，%后面必会跟上述三个参数之一），输出完打印的位置往下移动一位
kprintf:
	push   ebp
	mov    ebp, esp
	pusha
	xor    esi, esi
	mov    esi, [ebp+8]   ;打印起始位置	
	mov    ebx, [ebp+12]  ;字符串起始位置
	mov    ecx, 16        ;第三个参数位置
	mov    ah, 0Fh        ;打印白底黑字
	
;挨个字读字符串
ReadStr:
	push   eax
	mov    al, [ebx]
	inc    ebx
	cmp    al, 0
	jz     final
	cmp    al, '%'
	jz     c_f_b_case
	
	mov    edx, eax
	pop    eax
	mov    al, dl
	shl    esi,1          ;2字节
	mov    word[gs:esi], ax
	shr    esi,1
	inc    esi            ;输出位要后移动1位
	jmp    ReadStr

c_f_b_case:
	mov    al, [ebx]
	inc    ebx
	cmp    al,'c'
	jz     type_c
	cmp    al,'f'
	jz     type_f
	cmp    al,'b'
	jz     type_b

;character	
type_c:
	mov    dl, [ebp+ecx]    
	add    ecx, 4
	pop    eax
	mov    al, dl
	shl    esi, 1
	mov    word[gs:esi], ax
	shr    esi, 1
	inc    esi            ;输出位要后移动1位
	jmp    ReadStr
	
;frontground
type_f:
	mov    dl, [ebp+ecx]    
	add    ecx, 4
	pop    eax
	and    ah, 0xf0
	or     ah, dl         ;把dl和低4位清零的ah合并
	jmp    ReadStr
;background	
type_b:
	mov    dl, [ebp+ecx]    
	add    ecx, 4
	pop    eax
	and    ah, 0x0f
	shl    dl, 4          ;将背景色放在高四位
	or     ah, dl
	jmp    ReadStr
	
final:
	pop    eax
	popa
	mov    esp,ebp
	pop    ebp
	ret
