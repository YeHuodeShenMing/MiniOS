#include "list.h"

// 获得页入口
inline PageList getPageEntry(LinkList p)
{
	return ((PageList )((void *)(p) - ((void *)(&((PageList )0)->memmap_node))));
}

// 链表初始化
inline void InitList(LinkList head)
{
	head->next = head; 
	head->prev = head; 
}

// 头插
inline LinkList InsertNode(LinkList head, LinkList node)
{
    node->next = head;
    node->prev = head->prev;
    head->prev->next = node;
    head->prev = node;
    return node;
}

// 删节点
inline LinkList DeleNode(LinkList head, LinkList node)
{
    if (isEmptyList(head) || isEmptyList(node))
        return NULL;
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = node->next = NULL;
    return node;
}

inline bool isEmptyList(LinkList head)
{
	return (head == NULL || head->prev == NULL || head->next == NULL || head->prev == head || head->next == head);
}
