;初始代码 boot.asm  
    org    0x7c00            ; 告诉编译器程序加载到7c00处
    mov    ax, cs		;CS 是代码段寄存器，保存了代码段的基址。通过将 CS 的值复制给 AX，可以访问当前代码所在的段
    mov    ds, ax		;DS 是数据段寄存器，用于保存数据段的基址。通过将 AX 的值复制给 DS，可以设置数据段的基址与代码段相同，从而可以方便地访问数据
    mov    es, ax		;ES 是附加段寄存器，用于访问额外的数据段。在这里将 ES 设置为与 CS 相同的值，通常是为了方便在同一个地址空间中访问其他数据段
    call   DispStr           ; 调用显示字符串例程
    jmp    $                 ; 无限循环
DispStr:
    mov    ax, BootMessage
    mov    bp, ax            ; ES:BP = 串地址
    mov    cx, 16            ; CX = 串长度
    mov    ax, 1301h        ; AH = 13,  AL = 01h
    mov    bx, 000ch         ; 页号为0(BH = 0) 黑底红字(BL = 0Ch,高亮)
    mov    dh, 0
    mov    dl, 0
    int    10h               ; 10h 号中断
    ret
BootMessage:       db  "Hello, OS world!"
times  510-($-$$)  db  0     ; 填充剩下的空间，使生成的二进制代码恰好为512字节
dw     0xaa55                ; 结束标志
