#ifndef AF_DETAIL_CONTAINERS_LIST_H
#define AF_DETAIL_CONTAINERS_LIST_H

#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"


namespace AF
{
namespace Detail
{

/*
 * Class template implementing a generic unsorted list.
 * The item type is the node type and is expected to derive from Node.
 */
template <class ItemType>
class List {
public:
    /*
     * Baseclass that adds link members to node types that derive from it.
     */
    class Node {
    public:
        Node *next_;     // Pointer to the next item in the queue.
    };

    /*
     * Iterator type.
     */
    class Iterator {
    public:
        AF_FORCEINLINE Iterator() : node_(0) {
        }

        AF_FORCEINLINE explicit Iterator(Node *const node) : node_(node) {
        }

        AF_FORCEINLINE bool Next() {
            // On the first call this skips the head node, which is a dummy.
            node_ = node_->next_;
            return (node_ != 0);
        }

        AF_FORCEINLINE ItemType *Get() const
        {
            AF_ASSERT(node_);
            return static_cast<ItemType *>(node_);
        }

    protected:
        Node *node_;        // Pointer to the list node referenced by the iterator.
    };

    inline List();
    
    inline ~List();

    inline Iterator GetIterator() const;

    inline uint32_t Size() const;

    inline bool Empty() const;

    inline void Clear();

     // Adds an item to the list. The new item is referenced by pointer and is not copied.
    inline void Insert(ItemType *const item);

    inline bool Remove(ItemType *const item);

    inline ItemType *Front() const;

private:
    List(const List &other);
    List &operator=(const List &other);

    mutable Node head_;                 // Dummy element at the head of the list.
};


template <class ItemType>
AF_FORCEINLINE List<ItemType>::List() {
    head_.next_ = 0;
}

template <class ItemType>
AF_FORCEINLINE List<ItemType>::~List() {
    Clear();
}

template <class ItemType>
AF_FORCEINLINE typename List<ItemType>::Iterator List<ItemType>::GetIterator() const {
    return Iterator(&head_);
}

template <class ItemType>
AF_FORCEINLINE uint32_t List<ItemType>::Size() const {
    uint32_t count(0);
    Node *node(head_.next_);

    while (node) {
        ++count;
        node = node->next_;
    }

    return count;
}

template <class ItemType>
AF_FORCEINLINE bool List<ItemType>::Empty() const {
    return (head_.next_ == 0);
}

template <class ItemType>
AF_FORCEINLINE void List<ItemType>::Clear() {
    head_.next_ = 0;
}

template <class ItemType>
AF_FORCEINLINE void List<ItemType>::Insert(ItemType *const item) {
    item->next_ = head_.next_;
    head_.next_ = item;
}

template <class ItemType>
inline bool List<ItemType>::Remove(ItemType *const item) {
    // Find the node and the one before it, if any.
    Node *previous(&head_);
    Node *node(head_.next_);

    while (node) {
        // Note compared by address not value.
        if (node == item) {
            break;
        }

        previous = node;
        node = node->next_;
    }

    // Node in list?
    if (node) {
        // This works because we use a dummy as the head.
        previous->next_ = node->next_;
        return true;
    }

    // The caller owns the node so we don't need to free it.
    return false;
}

template <class ItemType>
AF_FORCEINLINE ItemType *List<ItemType>::Front() const {
    return static_cast<ItemType *>(head_.next_);
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_CONTAINERS_LIST_H
