#ifndef SWAP_H
#define SWAP_H

#include "type.h"
#include "string.h"
#include "memman.h"
#include "protect.h"
#include "proc.h"
#include "const.h"
#include "fs_const.h"
#include "fat32.h"
#include "const.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "hd.h"
#include "assert.h"
#include "stdio.h"
#include "fs.h"
#include "vfs.h"

#define PAGE_SIZE 4096
#define SWAP_PAGES 1024
#define SWAP_BITMAP_BYTES SWAP_PAGES / 8
#define SWAP_FILE_NAME "orange/swap.bin"

struct SwapInfo
{
    u32 version;			// 版本，仿照linux但是本题中并未用到
    u32 pages;				// swap区能够容纳页的数量
    u32 *bitmap;			// 位图，用来标记哪一个swap区的位置被使用了
    u32 next_AvailIndex;	// 用来知道下一个空闲的swap区中的页号是哪一个
};

void swapinit();
void clear_mem(u32 pde_phaddr, u32 pid);
void swapin(PageList pglist, u32 pde_phaddr);
void swapout(PageList pglist, u32 pde_phaddr);
void page_allocated_add(struct pgAllocatedInfo *temp);
void page_allocated_remove(struct pgAllocatedInfo *temp);

u32 swap_page_in(u32 vaddr);
void swap_page_out(u32 swap_idx, u32 vaddr);
u32 getAvailIndex(struct SwapInfo *swap);
struct pgAllocatedInfo get_pgSwapOut(u32 pid);
#endif 