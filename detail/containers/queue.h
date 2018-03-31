#ifndef AF_DETAIL_CONTAINERS_QUEUE_H
#define AF_DETAIL_CONTAINERS_QUEUE_H

#include "AF/align.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"


namespace AF
{
namespace Detail
{

/*
 * A fast unbounded queue.
 *
 * The queue is unbounded and internally is a doubly-linked, intrusive linked list.
 * The head and tail are stored as 'dummy' nodes to simplify insertion and removal.
 * The head and tail are separate nodes to separate the concerns of pushers and poppers.
 *
 * The queue is intrusive and the item type is expected to derive from Queue<ItemType>::Node.
 */
template <class ItemType>
class Queue {
public:
    /*
     * Baseclass that adds link members to node types that derive from it.
     * In order to be used with the queue, item classes must derive from Node.
     */
    class AF_PREALIGN(AF_CACHELINE_ALIGNMENT) Node {
    public:
        inline Node() : next_(0), prev_(0) {
        }

        Node *next_;        // Pointer to the next item in the list.
        Node *prev_;        // Pointer to the previous item in the list.

    private:
        Node(const Node &other);
        Node &operator=(const Node &other);

    } AF_POSTALIGN(AF_CACHELINE_ALIGNMENT);

    inline Queue();

    inline ~Queue();

    inline bool Empty() const;

    inline void Push(ItemType *const item);

    inline ItemType *Front() const;

    inline ItemType *Pop();

private:
    Queue(const Queue &other);
    Queue &operator=(const Queue &other);

    Node head_;     // Dummy node that is always the head of the list.
    Node tail_;     // Dummy node that is always the tail of the list.
};


template <class ItemType>
AF_FORCEINLINE Queue<ItemType>::Queue() : head_(), tail_() {
    head_.next_ = 0;
    head_.prev_ = &tail_;

    tail_.next_ = &head_;
    tail_.prev_ = 0;
}

template <class ItemType>
AF_FORCEINLINE Queue<ItemType>::~Queue() {
    // If the queue hasn't been emptied by the caller we'll leak the nodes.
    AF_ASSERT(head_.next_ == 0);
    AF_ASSERT(head_.prev_ == &tail_);

    AF_ASSERT(tail_.next_ == &head_);
    AF_ASSERT(tail_.prev_ == 0);
}

template <class ItemType>
AF_FORCEINLINE bool Queue<ItemType>::Empty() const {
    return (head_.prev_ == &tail_);
}

template <class ItemType>
AF_FORCEINLINE void Queue<ItemType>::Push(ItemType *const item) {
    // Doubly-linked list insert at back, ie. in front of the dummy tail.
    item->prev_ = &tail_;
    item->next_ = tail_.next_;

    tail_.next_->prev_= item;
    tail_.next_ = item;
}

template <class ItemType>
AF_FORCEINLINE ItemType *Queue<ItemType>::Front() const {
    // It's illegal to call Front when the queue is empty.
    AF_ASSERT(head_.prev_ != &tail_);
    return static_cast<ItemType *>(head_.prev_);
}

template <class ItemType>
AF_FORCEINLINE ItemType *Queue<ItemType>::Pop() {
    Node *const item(head_.prev_);

    // It's illegal to call Pop when the queue is empty.
    AF_ASSERT(item != &tail_);

    // Doubly-linked list remove from front, ie. behind the dummy head.
    item->prev_->next_ = &head_;
    head_.prev_ = item->prev_;

    return static_cast<ItemType *>(item);
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_CONTAINERS_QUEUE_H
