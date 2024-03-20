;int 10h    
    org    0x7c00            ; 告诉编译器程序加载到7c00处
    mov    ax, cs
    mov    ds, ax
    mov    es, ax
    call   Clear
    call   DispStr           ; 调用显示字符串例程
    call   Cursor
    jmp    $                 ; 无限循环
    
    
Clear:
    mov    ax,0600h
    mov    ch, 0
    mov    cl, 0
    mov    dh, 8
    mov    dl, 79	     ;结束行、列为50和80
    int    10h               ; 10h 号中断
    ret

    
DispStr:
    mov    ax, BootMessage
    mov    bp, ax            ; ES:BP = 串地址
    mov    cx, 4            ; CX = 串长度
    mov    ax, 1301h        ; AH = 13,  AL = 01h
    mov    bx, 001fh         ; 页号为0(BH = 0) 蓝底白字(BL = 1Fh,高亮)
    mov    dh, 18
    mov    dl, 37
    int    10h               ; 10h 号中断
    ret

Cursor:
    mov    ah, 02h          ; AH = 02h 表示请求移动光标的功能
    mov    bh, 0            ; 页号为0
    mov    dh, 9            ; 设置光标所在行为10（从0开始计数）
    mov    dl, 9            ; 设置光标所在列为10（从0开始计数）
    int    10h              ; 10h 号中断
    ret
    
BootMessage:       db  "NWPU"
times  510-($-$$)  db  0     ; 填充剩下的空间，使生成的二进制代码恰好为512字节
dw     0xaa55                ; 结束标志
