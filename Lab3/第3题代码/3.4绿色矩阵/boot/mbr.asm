; Struct for Partition Table Entry

struc PTEntry

    .status     resb    1 ; bit 7 set = active

    .startCHS   resb    3

    .type       resb    1

    .endCHS     resb    3

    .startLBA   resb    4

    .numSectors resb    4

endstruc

PTOffset        equ 446      ;分页表相对偏移

OSBOOTSegment   equ 0000h    ;BaseOfBoot 					equ	1000h 		; 段地址

OSBOOTOffset    equ 7c00h    ;活动主分区  ;OffsetOfBoot		equ	7c00h		

    

MBROffset       equ 0800h

ActiveSectionOffset    equ  7e00h      ; 活动分区的起始扇区号相对于BaseOfBoot的偏移量

PART_TYPE_EXT   equ 5

org MBROffset

START:

    cli ; turn off interrupt for safety, though in most cases this is trivial

    xor     ax, ax

    mov     ds, ax             ;数据段寄存器（data segment）清0

    mov     es, ax             ;附加段寄存器（extra segment）清0

    mov     ss, ax             ;栈段寄存器（stack segment）清0

    mov     sp, 7c00h

    ; copy self to another space, since the boot expects it to be at 0x7c00 where we are now

    mov     cx, 256

    mov     si, 7c00h

    mov     di, MBROffset

    rep movsw ; copy cx words from [si] to [di]

    jmp     0:REAL_START ; mbr is copied, use an explicit jump to switch the IP(Instruction Pointer)

REAL_START:

    sti ; turn on interrupt, or BIOS INTs cannot work

    ; Save boot drive number passed from BIOS

    mov     [BOOT_DRIVE_NUMBER], dl

    ; Clear screen

    mov     ax, 0600h

    mov     bx, 0700h

    mov     cx, 0       ; LeftTop(row, col) = (0, 0)

    mov     dx, 184fh   ; RightBottom(row, col) = (24, 79)

    int     10h

    ; Reset Cursor to (0, 0)

    mov     ax, 0200h

    mov     bx, 0

    mov     dx, 0000h

    int     10h

    ; Print "Find partition"

    mov    ax, 01301h                       ; AH = 13,  AL = 01h

    mov    bx, 000Fh                        ; 页号为0(BH = 0) 黑底白字(BL = 0Bh,高亮)

    mov    cx, MSG_FindingEnd-MSG_Finding   ; CX = 串长度

    xor    dx, dx                           ; DX = (起始行|列)

    mov    bp, MSG_Finding                  ; ES:BP = 串地址

    int    10h                              ; 10h 号中断

 

; TODO1.3&1.4: Add/Modify code below for parsing Partition Table & finding the first active primary partition

    mov     ax,MBROffset

    mov     si,ax
	;mov     ds,ax
	;;ds  ds ds!!!

    add     si, PTOffset       ; 将SI寄存器设置为分区表的偏移量

    mov     cx, 4              ; 设置循环计数器为4，因为分区表有4个条目

    ;循环读取分区表

    mov	    bx, MBROffset+PTOffset				;每次循环累加16

    mov	    dh, 1				;每次循环累加1


PARSE_PARTITION_TABLE:

    ; 解析分区表项

    ;mov     di, PTEntry.status ; 将DI寄存器设置为分区表项的状态字段

    ;mov     dl, byte[si+bx]           ; 从分区表中读取一个字节"分区状态"到DL寄存器

    ;mov     cl, byte[si+bx+4]           ; 从分区表中读取一个字节"分区类型"到AL寄存器

	;mov     eax, dword[si+bx+8]        ;"分区起始扇区地址"

	

	mov     dl, byte[bx+PTEntry.status]           ; 从分区表中读取一个字节"分区状态"到DL寄存器
	mov     al,dl
	movzx   eax,al
	call   DispInt32

    mov     al, byte[bx+PTEntry.type]           ; 从分区表中读取一个字节"分区类型"到AL寄存器
		movzx     eax,al
	call   DispInt32

	mov     eax, dword[bx+PTEntry.startLBA]        ;"分区起始扇区地址"
	call   DispInt32

	cmp		dl, 80h           ; 检查状态的最高位是否为1 

	jz		LABLE_ACTIVE

         

    ;add     si, 4 

    ;mov     al, byte[si+bx+4]           ; 从分区表中读取一个字节分区类型到AL寄存器	

    ;movzx   eax,al

	

	;and    eax,0x000000ff

    ;call   DispInt32

    ;add     si, 4 

    ;mov     eax, dword[si+bx+8]          ; 从分区表中读取一个双字到EAX寄存器

    ;call    DispInt32	

    ;add     si, 8              ; 增加SI和DI以便处理下一个分区表项

    ;\r输入一个换行符，便于观察

    ;mov    ah,0x0e

    ;mov    al,0ah

    ;int    10h

    ;dec    cx

	inc		dh

	add		bx, 10h

    loop    PARSE_PARTITION_TABLE  ; 循环处理下一个分区表项
	;jmp $

    ;jmp     $
LABLE_ACTIVE:

	
	;mov     dword [es:OffsetOfActiPartStartSec], eax 	
	push   eax
	sub    bx,0800h
	sub    bx,446
	mov    ax,bx
	mov    bl,16
	div    bl
	inc    al
	movzx  eax,al
	call   DispInt32
	pop    eax
	;jmp   $

	mov 	bx, OSBOOTOffset ;对应扇区会被加载到内存的 es:bx 处

	call 	LOAD_OSBOOT
	
	



    jmp     OSBOOTSegment:OSBOOTOffset   

LOAD_OSBOOT:

; TODO1.4: You need to set INT13H's DAP to read the boot program into memory

    ;add     si, 8 ;将si寄存器设置为分区表项的起始LBA字段

        ;mov     eax, dword[si]          ; 从分区表中读取一个双字到EAX寄存器

	;movzx   eax,ax
	mov     si,DAPS

	mov     dword [si+8], eax    ; 将startLBA字段的值保存在LBA_Lo变量中

	;mov     eax, dword [si + 4]    ; 读取startLBA字段的高32位值到EAX寄存器

	;and     eax,0x0000ffff

	;mov     dword [LBA_Hi], eax    ; 将startLBA字段的高32位值保存在LBA_Hi变量中

       mov     dl, [BOOT_DRIVE_NUMBER]

	mov     ah,42h

	

	int     13h

	ret

    ; mov     cx, MSG_ReadFailureEnd-MSG_ReadFailure

    ; mov     bp, MSG_ReadFailure

    ; jc      EXIT_FAIL

; TODO1.4: You need to ensure that `DS:SI` points to the Partition Table Entry associated 

;       with the boot you are going to load at this point.

; Hint1: You will find it useful in Task2.1

 

EXIT_FAIL:

    mov    ax, 01301h

    mov    bx, 000ch

    mov    dx, 0100h

    int    10h

    jmp $

; Data Address Packet

DAPS:   

    DB 0x10                     ; size of packet 

    DB 0                        ; Always 0

    D_CL    DW 1                ; number of sectors to transfer

    D_BX    DW OSBOOTOffset     ; transfer buffer (16 bit segment:16 bit offset) 

    D_ES    DW OSBOOTSegment    

    LBA_Lo  DD 1                ; lower 32-bits of 48-bit starting LBA

    LBA_Hi  DD 0                ; upper 32-bits of 48-bit starting LBAs

; -----------------

BOOT_DRIVE_NUMBER   db 00h

; -----------------

MSG_Finding:        db "FindPart"

MSG_FindingEnd:
ActivePart_Number:        db "FindPart"

ActivePart_NumberEnd:

MSG_NotFound:       db "NoPart"

MSG_NotFoundEnd:

MSG_ReadFailure:    db "RdFail"

MSG_ReadFailureEnd:

; -----------------

; The code snippet below is for your reference, in case you don't want to waste

; 	your time on figuring out how to print a 32-bit integer

; -----------------

DispInt32:

     pusha

    xor di, di  ; $di counts how many digits are there

    mov ecx, 10

.loop1:

    xor edx, edx

    ;and eax,0xff

    div ecx     ; {edx, eax} / ecx(=10) -> eax ... edx

    push dx

    inc di

    test eax,eax

    jnz .loop1

.loop2:

    pop ax

    

    mov ah, 0Eh

    

    add al, '0'

   

    mov bl, 0Fh

    int 10H     ; INT 10H & AH = 0EH, print a character

                ;           AL = character to write

                ;           BH = page number

                ;           BL = foreground color (graphics modes only)

    dec di

    test di, di

    jnz .loop2

    mov    ah,0x0e

    mov    al,' '

    int    10h

    popa

    ret

    ; DispAH:


	; pusha


	; add al,'0'

	; mov dl,al

	; mov ah,2

	; int 21h

	; mov    ah,0x0e

	; mov    al,space

	; int    10h

	; popa

	; ret

SecOffset_SELF 	dd 	0

SecOffset_EXT 	dd 	0

EbrNum		db	0

FirstInExt	db  0 

EndInExt	db	0

EXTNum		db  0 

CurPartNum  db  0 

CurPartNo	db	"0"

; ----------------- Padding ----------------

times 446 -($-$$) db 0  ; MBR[0:445] = Boot code

                        ; MBR[446:509] = Partition Table

                        ; MBR[510:511] = 0x55aa
