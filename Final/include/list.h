#ifndef LIST_H
#define LIST_H

#include "type.h"

// file information
struct FileInfo 
{
    int fd;
    u32 offset;
    u32 size;
};

typedef struct LinkNode 
{
    struct LinkNode *prev;
    struct LinkNode *next;
}*LinkList;

typedef struct PageNode 
{
    u8 flag;
    u32 swap_index;
	u32 segment;
    u32 v_addr;
	u32 p_addr;
    struct FileInfo fileinfo;
    struct LinkNode page_node;
    struct LinkNode memmap_node;  // 描述mm的类型
}*PageList;


PageList  getPageEntry(LinkList p);
void InitList(LinkList head);
LinkList InsertNode(LinkList head, LinkList node);
LinkList DeleNode(LinkList head, LinkList node);
bool isEmptyList(LinkList head);

#endif 
