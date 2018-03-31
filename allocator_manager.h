#ifndef AF_ALLOCATORMANAGER_H
#define AF_ALLOCATORMANAGER_H

#include "AF/allocator_interface.h"
#include "AF/assert.h"
#include "AF/default_allocator.h"
#include "AF/defines.h"

#include "AF/detail/allocators/caching_allocator.h"

#include "AF/detail/threading/spin_lock.h"


namespace AF
{

class AllocatorManager
{
public:
    static void SetAllocator(AllocatorInterface *const allocator);

    AF_FORCEINLINE static AllocatorInterface *GetAllocator() {
        return cache_.GetAllocator();
    }

     // Gets a pointer to the caching allocator that wraps the general allocator.
    AF_FORCEINLINE static AllocatorInterface *GetCache() {
        return &cache_;
    }

private:
    struct CacheTraits {
        typedef Detail::SpinLock LockType;

        struct AF_PREALIGN(AF_CACHELINE_ALIGNMENT) AlignType {
        } AF_POSTALIGN(AF_CACHELINE_ALIGNMENT);

        static const uint32_t MAX_POOLS = 8;
        static const uint32_t MAX_BLOCKS = 16;
    };

    typedef Detail::CachingAllocator<CacheTraits> CacheType;

    AF_FORCEINLINE AllocatorManager() {
    }

    AllocatorManager(const AllocatorManager &other);
    AllocatorManager &operator=(const AllocatorManager &other);

    static DefaultAllocator default_allocator_;     // Default allocator used if no user allocator is set.
    static CacheType cache_;                        // Cache that caches allocations from the actual allocator.
};


} // namespace AF



#endif // AF_ALLOCATORMANAGER_H
