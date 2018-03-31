#ifndef AF_DETAIL_ALLOCATORS_POOL_H
#define AF_DETAIL_ALLOCATORS_POOL_H

#include "AF/align.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"


namespace AF
{
namespace Detail
{

/*
 * A pool of free memory blocks.
 */
template <uint32_t MAX_BLOCKS>
class Pool {
public:
    inline Pool();

    inline bool Empty() const;

    inline bool Add(void *memory);

    /*
     * Retrieves a memory block from the pool with the given alignment.
     * Return 0 if no suitable blocks in pool.
     */
    inline void *FetchAligned(const uint32_t alignment);

    /*
     * Retrieves a memory block from the pool with any alignment.
     * Return 0 if no blocks in pool.
     */
    inline void *Fetch();

private:
    /*
     * A node representing a free memory block within the pool.
     * Nodes are created in-place within the free blocks they represent.
     */
    struct Node {
        AF_FORCEINLINE Node() : next_(0) {
        }

        Node *next_;        // Pointer to next node in a list.
    };

    Node head_;             // Dummy node at head of a linked list of nodes in the pool.
    uint32_t block_count_;  // Number of blocks currently cached in the pool.
};


template <uint32_t MAX_BLOCKS>
AF_FORCEINLINE Pool<MAX_BLOCKS>::Pool() 
  : head_(),
    block_count_(0) {
}

template <uint32_t MAX_BLOCKS>
AF_FORCEINLINE bool Pool<MAX_BLOCKS>::Empty() const {
    AF_ASSERT((block_count_ == 0 && head_.next_ == 0) || (block_count_ != 0 && head_.next_ != 0));
    return (block_count_ == 0);
}

template <uint32_t MAX_BLOCKS>
AF_FORCEINLINE bool Pool<MAX_BLOCKS>::Add(void *const memory) {
    AF_ASSERT(memory);

    // Below maximum block count limit?
    if (block_count_ < MAX_BLOCKS) {
        // Just call it a node and link it in.
        Node *const node(reinterpret_cast<Node *>(memory));

        node->next_ = head_.next_;
        head_.next_ = node;

        ++block_count_;
        return true;
    }

    return false;
}

template <uint32_t MAX_BLOCKS>
AF_FORCEINLINE void *Pool<MAX_BLOCKS>::FetchAligned(const uint32_t alignment) {
    Node *previous(&head_);
    const uint32_t alignment_mask(alignment - 1);

    // Search the block list.
    Node *node(head_.next_);
    while (node) {
        Node *const next(node->next_);

        if ((reinterpret_cast<uintptr_t>(node) & alignment_mask) == 0) {
            // Remove from list and return as a block.
            previous->next_ = next;
            --block_count_;

            return reinterpret_cast<void *>(node);
        }

        previous = node;
        node = next;
    }

    // 0 result indicates no correctly aligned block available.
    return 0;
}

template <uint32_t MAX_BLOCKS>
AF_FORCEINLINE void *Pool<MAX_BLOCKS>::Fetch() {
    // Grab first block in the list if the list isn't empty.
    Node *const node(head_.next_);
    if (node) {
        head_.next_ = node->next_;
        --block_count_;

        return reinterpret_cast<void *>(node);
    }

    // 0 result indicates no block available.
    return 0;
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_ALLOCATORS_POOL_H

