#include <lib/list.h>
#include <kernel/assert.h>

// 初始化链表
void listInit(list_t *list)
{
    list->head.prev = nullptr;
    list->tail.next = nullptr;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
}

// 在 anchor 结点前插入结点 node
void listInsertBefore(list_node_t *anchor, list_node_t *node)
{
    node->prev = anchor->prev;
    node->next = anchor;

    anchor->prev->next = node;
    anchor->prev = node;
}

// 在 anchor 结点后插入结点 node
void listInsertAfter(list_node_t *anchor, list_node_t *node)
{
    node->prev = anchor;
    node->next = anchor->next;

    anchor->next->prev = node;
    anchor->next = node;
}

// 插入到头结点后
void listPush(list_t *list, list_node_t *node)
{
    assert(!listSearch(list, node), "The inserted node exists.");
    listInsertAfter(&list->head, node);
}

// 移除头结点后的结点
list_node_t *listPop(list_t *list)
{
    assert(!listEmpty(list), "The removed node does not exist.");

    list_node_t *node = list->head.next;
    listRemove(node);

    return node;
}

// 插入到尾结点前
void listPushback(list_t *list, list_node_t *node)
{
    assert(!listSearch(list, node), "The inserted node exists.");
    listInsertBefore(&list->tail, node);
}

// 移除尾结点前的结点
list_node_t *listPopback(list_t *list)
{
    assert(!listEmpty(list), "The removed node does not exist.");

    list_node_t *node = list->tail.prev;
    listRemove(node);

    return node;
}

// 查找链表中结点是否存在
bool listSearch(list_t *list, list_node_t *node)
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
void listRemove(list_node_t *node)
{
    assert(node->prev != nullptr, "The precursor of the deleted node is null.");
    assert(node->next != nullptr, "The rear drive of the deleted node is empty.");

    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = nullptr;
    node->prev = nullptr;
}

// 判断链表是否为空
bool listEmpty(list_t *list)
{
    return (list->head.next == &list->tail);
}

// 获得链表长度
uint32 listSize(list_t *list)
{
    list_node_t *next = list->head.next;
    uint32 size = 0;
    while (next != &list->tail)
    {
        size++;
        next = next->next;
    }
    return size;
}

// 链表插入排序
void listInsertSort(list_t *list, list_node_t *node, int offset)
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
