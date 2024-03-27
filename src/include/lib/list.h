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
} list_node_t;

// 链表
typedef struct list_t
{
    list_node_t head; // 头结点
    list_node_t tail; // 尾结点
} list_t;

// 初始化链表
void listInit(list_t *list);

// 在 anchor 结点前插入结点 node
void listInsertBefore(list_node_t *anchor, list_node_t *node);

// 在 anchor 结点后插入结点 node
void listInsertAfter(list_node_t *anchor, list_node_t *node);

// 插入到头结点后
void listPush(list_t *list, list_node_t *node);

// 移除头结点后的结点
list_node_t *listPop(list_t *list);

// 插入到尾结点前
void listPushback(list_t *list, list_node_t *node);

// 移除尾结点前的结点
list_node_t *listPopback(list_t *list);

// 查找链表中结点是否存在
bool listSearch(list_t *list, list_node_t *node);

// 从链表中删除结点
void listRemove(list_node_t *node);

// 判断链表是否为空
bool listEmpty(list_t *list);

// 获得链表长度
uint32 listSize(list_t *list);

// 链表插入排序
void listInsertSort(list_t *list, list_node_t *node, int offset);

#endif