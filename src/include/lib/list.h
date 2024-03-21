#ifndef SOUL_LIST_H
#define SOUL_LIST_H

#include <types/types.h>


#define ELEMENT_OFFSET(type, member) (uint32)(&((type *)0)->member)
#define ELEMENT_ENTRY(type, member, ptr) (type *)((uint32)ptr - ELEMENT_OFFSET(type, member))
#define ELEMENT_NODE_OFFSET(type, node, key) ((int)(&((type *)0)->key) - (int)(&((type *)0)->node))
#define ELEMENT_NODE_KEY(node, offset) *(int *)((int)node + offset)


// 链表结点
typedef struct list_node_t
{
    struct list_node_t *prev; // 下一个结点
    struct list_node_t *next; // 前一个结点
} LIST_NODE_T;

// 链表
typedef struct list_t
{
    LIST_NODE_T head; // 头结点
    LIST_NODE_T tail; // 尾结点
} LIST_T;

// 初始化链表
void listInit(LIST_T *list);

// 在 anchor 结点前插入结点 node
void listInsertBefore(LIST_NODE_T *anchor, LIST_NODE_T *node);

// 在 anchor 结点后插入结点 node
void listInsertAfter(LIST_NODE_T *anchor, LIST_NODE_T *node);

// 插入到头结点后
void listPush(LIST_T *list, LIST_NODE_T *node);

// 移除头结点后的结点
LIST_NODE_T *listPop(LIST_T *list);

// 插入到尾结点前
void listPushback(LIST_T *list, LIST_NODE_T *node);

// 移除尾结点前的结点
LIST_NODE_T *listPopback(LIST_T *list);

// 查找链表中结点是否存在
bool listSearch(LIST_T *list, LIST_NODE_T *node);

// 从链表中删除结点
void listRemove(LIST_NODE_T *node);

// 判断链表是否为空
bool listEmpty(LIST_T *list);

// 获得链表长度
uint32 listSize(LIST_T *list);

// 链表插入排序
void listInsertSort(LIST_T *list, LIST_NODE_T *node, int offset);

#endif