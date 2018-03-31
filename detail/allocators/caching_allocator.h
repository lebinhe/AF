#ifndef AF_DETAIL_ALLOCATORS_CACHINGALLOCATOR_H
#define AF_DETAIL_ALLOCATORS_CACHINGALLOCATOR_H

#include "AF/align.h"
#include "AF/allocator_interface.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/allocators/pool.h"


namespace AF
{
namespace Detail
{

struct DefaultCacheTraits {
    class LockType {
    public:
        AF_FORCEINLINE void Lock() {
        }

        AF_FORCEINLINE void Unlock() {
        }
    };

    struct AlignType {
    };

    static const uint32_t MAX_POOLS = 8;
    static const uint32_t MAX_BLOCKS = 16;
};


/*
 * A thread-safe caching allocator that caches free memory blocks.
 */
template <class CacheTraits = DefaultCacheTraits>
class CachingAllocator : public AF::AllocatorInterface {
public:
    inline CachingAllocator();

    inline explicit CachingAllocator(AllocatorInterface *const allocator);

    inline virtual ~CachingAllocator();

    inline void SetAllocator(AllocatorInterface *const allocator);

    inline AllocatorInterface *GetAllocator() const;

    inline virtual void *Allocate(const uint32_t size);

    inline virtual void *AllocateAligned(const uint32_t size, const uint32_t alignment);

    inline virtual void Free(void *const block) override;

    inline virtual void FreeWithSize(void *const block, const SizeType /* size */) override;

    inline void Clear();

private:
    class Entry {
    public:
        typedef Detail::Pool<CacheTraits::MAX_BLOCKS> PoolType;

        inline Entry() : block_size_(0) {
        }

        typename CacheTraits::AlignType align_;
        uint32_t block_size_;
        PoolType pool_;
    };

    CachingAllocator(const CachingAllocator &other);
    CachingAllocator &operator=(const CachingAllocator &other);

    AllocatorInterface *allocator_;            // pointer to a wrapped low-level allocator.
    typename CacheTraits::LockType lock_;      // protects access to the pools.
    Entry entries_[CacheTraits::MAX_POOLS];    // pools of memory blocks of different sizes.
};


template <class CacheTraits>
AF_FORCEINLINE CachingAllocator<CacheTraits>::CachingAllocator() 
  : allocator_(0) {
}


template <class CacheTraits>
AF_FORCEINLINE CachingAllocator<CacheTraits>::CachingAllocator(AllocatorInterface *const allocator) 
  : allocator_(allocator) {
}


template <class CacheTraits>
AF_FORCEINLINE CachingAllocator<CacheTraits>::~CachingAllocator() {
    Clear();
}

template <class CacheTraits>
inline void CachingAllocator<CacheTraits>::SetAllocator(AllocatorInterface *const allocator) {
    allocator_ = allocator;
}

template <class CacheTraits>
inline AllocatorInterface *CachingAllocator<CacheTraits>::GetAllocator() const {
    return allocator_;
}

template <class CacheTraits>
inline void *CachingAllocator<CacheTraits>::Allocate(const uint32_t size) {
    return AllocateAligned(size, sizeof(void *));
}

template <class CacheTraits>
inline void *CachingAllocator<CacheTraits>::AllocateAligned(const uint32_t size, const uint32_t alignment) {
    // Clamp small allocations to at least the size of a pointer.
    const uint32_t block_size(size >= sizeof(void *) ? size : sizeof(void *));
    void *block(0);

    // Alignment values are expected to be powers of two and at least 4 bytes.
    AF_ASSERT(block_size >= 4);
    AF_ASSERT(alignment >= 4);
    AF_ASSERT((alignment & (alignment - 1)) == 0);

    lock_.Lock();

    // The last pool is reserved and should always be empty.
    AF_ASSERT(entries_[CacheTraits::MAX_POOLS - 1].block_size_ == 0);
    AF_ASSERT(entries_[CacheTraits::MAX_POOLS - 1].pool_.Empty());

    // Search each entry in turn for one whose pool contains blocks of the required size.
    // Stop if we reach the first unused entry (marked by a block size of zero).
    uint32_t index(0);
    while (index < CacheTraits::MAX_POOLS) {
        Entry &entry(entries_[index]);
        if (entry.block_size_ == block_size) {
            // Try to allocate a block from the pool.
            block = entry.pool_.FetchAligned(alignment);
            break;
        }

        // Found the first unused pool or the very last pool?
        if (entry.block_size_ == 0) {
            // Reserve it for blocks of the current size.
            AF_ASSERT(entry.pool_.Empty());
            entry.block_size_ = block_size;
            break;
        }

        ++index;
    }

    // Swap the pool with that of lower index, if it isn't already first.
    // This is a kind of least-recently-requested replacement policy for pools.
    if (index > 0) {
        Entry temp(entries_[index]);
        entries_[index] = entries_[index - 1];
        entries_[index - 1] = temp;
    }

    // If we claimed the last pool then clear the pool which is
    // Now last after the swap, so it's available for next time.
    if (index == CacheTraits::MAX_POOLS - 1) {
        Entry &entry(entries_[CacheTraits::MAX_POOLS - 1]);
        entry.block_size_ = 0;

        while (!entry.pool_.Empty()) {
            allocator_->FreeWithSize(entry.pool_.Fetch(), entry.block_size_);
        }
    }

    // Check that the last pool has been left unused and empty.
    AF_ASSERT(entries_[CacheTraits::MAX_POOLS - 1].block_size_ == 0);
    AF_ASSERT(entries_[CacheTraits::MAX_POOLS - 1].pool_.Empty());

    lock_.Unlock();

    if (block == 0) {
        // Allocate a new block.
        block = allocator_->AllocateAligned(block_size, alignment);
    }

    return block;
}

template <class CacheTraits>
inline void CachingAllocator<CacheTraits>::Free(void *const block) {
    // We don't try to cache blocks of unknown size.
    allocator_->Free(block);
}

template <class CacheTraits>
inline void CachingAllocator<CacheTraits>::FreeWithSize(void *const block, const SizeType size) {
    // Small allocations are clamped to at least the size of a pointer.
    const uint32_t block_size(size >= sizeof(void *) ? size : sizeof(void *));
    bool added(false);

    AF_ASSERT(block_size >= 4);
    AF_ASSERT(block);

    lock_.Lock();

    // Search each entry in turn for one whose pool is for blocks of the given size.
    // Stop if we reach the first unused entry (marked by a block size of zero).
    // Don't search the very last pool, since it is reserved for use in swaps.
    uint32_t index(0);
    while (index < CacheTraits::MAX_POOLS && entries_[index].block_size_) {
        Entry &entry(entries_[index]);
        if (entry.block_size_ == block_size) {
            // Try to add the block to the pool, if it's not already full.
            added = entry.pool_.Add(block);
            break;
        }

        ++index;
    }

    lock_.Unlock();

    if (!added) {
        // No pools are assigned to blocks of this size; free it.
        allocator_->FreeWithSize(block, block_size);
    }
}

template <class CacheTraits>
AF_FORCEINLINE void CachingAllocator<CacheTraits>::Clear() {
    lock_.Lock();

    // Free any remaining blocks in the pools.
    for (uint32_t index = 0; index < CacheTraits::MAX_POOLS; ++index) {
        Entry &entry(entries_[index]);
        while (!entry.pool_.Empty()) {
            allocator_->Free(entry.pool_.Fetch());
        }
    }

    lock_.Unlock();
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_ALLOCATORS_CACHINGALLOCATOR_H
