#include "AF/allocator_manager.h"


namespace AF
{

DefaultAllocator AllocatorManager::default_allocator_;
AllocatorManager::CacheType AllocatorManager::cache_(&default_allocator_);


void AllocatorManager::SetAllocator(AllocatorInterface *const allocator) {
    // This method should only be called once, at start of day.
    AF_ASSERT_MSG(default_allocator_.GetBytesAllocated() == 0, "SetAllocator can't be called while AF objects are alive");

    // We don't bother to make this thread-safe because it should only be called at start-of-day.
    if (allocator) {
        cache_.SetAllocator(allocator);
    } else {
        cache_.SetAllocator(&default_allocator_);
    }
}


} // namespace AF


