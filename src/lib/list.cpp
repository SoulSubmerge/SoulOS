#include <lib/list.h>
#include <kernel/assert.h>

// 初始化链表
void listInit(LIST_T *list)
{
    list->head.prev = nullptr;
    list->tail.next = nullptr;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
}

// 在 anchor 结点前插入结点 node
void listInsertBefore(LIST_NODE_T *anchor, LIST_NODE_T *node)
{
    node->prev = anchor->prev;
    node->next = anchor;

    anchor->prev->next = node;
    anchor->prev = node;
}

// 在 anchor 结点后插入结点 node
void listInsertAfter(LIST_NODE_T *anchor, LIST_NODE_T *node)
{
    node->prev = anchor;
    node->next = anchor->next;

    anchor->next->prev = node;
    anchor->next = node;
}

// 插入到头结点后
void listPush(LIST_T *list, LIST_NODE_T *node)
{
    assert(!listSearch(list, node), "The inserted node exists.");
    listInsertAfter(&list->head, node);
}

// 移除头结点后的结点
LIST_NODE_T *listPop(LIST_T *list)
{
    assert(!listEmpty(list), "The removed node does not exist.");

    LIST_NODE_T *node = list->head.next;
    listRemove(node);

    return node;
}

// 插入到尾结点前
void listPushback(LIST_T *list, LIST_NODE_T *node)
{
    assert(!listSearch(list, node), "The inserted node exists.");
    listInsertBefore(&list->tail, node);
}

// 移除尾结点前的结点
LIST_NODE_T *listPopback(LIST_T *list)
{
    assert(!listEmpty(list), "The removed node does not exist.");

    LIST_NODE_T *node = list->tail.prev;
    listRemove(node);

    return node;
}

// 查找链表中结点是否存在
bool listSearch(LIST_T *list, LIST_NODE_T *node)
{
    list_node_t *next = list->head.next;
    while (next != &list->tail)
    {
        if (next == node)
            return true;
        next = next->next;
    }
    return false;
}

// 从链表中删除结点
void listRemove(LIST_NODE_T *node)
{
    assert(node->prev != nullptr, "The precursor of the deleted node is null.");
    assert(node->next != nullptr, "The rear drive of the deleted node is empty.");

    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = nullptr;
    node->prev = nullptr;
}

// 判断链表是否为空
bool listEmpty(LIST_T *list)
{
    return (list->head.next == &list->tail);
}

// 获得链表长度
uint32 listSize(LIST_T *list)
{
    LIST_NODE_T *next = list->head.next;
    uint32 size = 0;
    while (next != &list->tail)
    {
        size++;
        next = next->next;
    }
    return size;
}

// 链表插入排序
void listInsertSort(list_t *list, LIST_NODE_T *node, int offset)
{
    // 从链表找到第一个比当前节点 key 点更大的节点，进行插入到前面
    list_node_t *anchor = &list->tail;
    int key = ELEMENT_NODE_KEY(node, offset);
    for (list_node_t *ptr = list->head.next; ptr != &list->tail; ptr = ptr->next)
    {
        int compare = ELEMENT_NODE_KEY(ptr, offset);
        if (compare > key)
        {
            anchor = ptr;
            break;
        }
    }

    assert(node->next == nullptr, "The rear drive of the node to be inserted is not empty.");
    assert(node->prev == nullptr, "The precursor of the node to be inserted is not empty.");

    // 插入链表
    listInsertBefore(anchor, node);
}
