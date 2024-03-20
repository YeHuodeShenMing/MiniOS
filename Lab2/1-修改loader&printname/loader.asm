org 0400h

mov ax, 0B800h
mov gs, ax


call    DispStr
jmp	$				; 到此停住

DispStr:
    mov    ax, BootMessage
    mov    bp, ax            ; ES:BP = 串地址
    mov    cx, 26            ; CX = 串长度
    mov    ax, 01301h        ; AH = 13,  AL = 01h
    mov    bx, 000Ah         ; 页号为0(BH = 0) 黑底红字(BL = 0Ch,高亮)
    mov    dl, 0
    int    10h               ; 10h 号中断
    ret
BootMessage:             db    "This is LIUJIACHEN's boot!"


