#include "AF/defines.h"

#include "AF/assert.h"

#include "AF/detail/strings/string_pool.h"

#include "AF/detail/threading/lock.h"

namespace AF
{
namespace Detail
{


StringPool *StringPool::instance_ = 0;
Mutex StringPool::reference_mutex_;
uint32_t StringPool::reference_count_ = 0;


void StringPool::Reference() {
    Lock lock(reference_mutex_);

    // Create the singleton instance if this is the first reference.
    if (reference_count_++ == 0) {
        AllocatorInterface *const allocator(AllocatorManager::GetCache());
        void *const memory(allocator->AllocateAligned(sizeof(StringPool), AF_CACHELINE_ALIGNMENT));
        instance_ = new (memory) StringPool();
    }
}

void StringPool::Dereference() {
    Lock lock(reference_mutex_);

    // Destroy the singleton instance if this was the last reference.
    if (--reference_count_ == 0) {
        AllocatorInterface *const allocator(AllocatorManager::GetCache());
        instance_->~StringPool();
        allocator->FreeWithSize(instance_, sizeof(StringPool));
    }
}

StringPool::StringPool() {
}

StringPool::~StringPool() {
}

const char *StringPool::Lookup(const char *const str) {
    // Hash the string value to a bucket index.
    const uint32_t index(Hash(str));
    AF_ASSERT(index < BUCKET_COUNT);
    Bucket &bucket(buckets_[index]);

    // Lock the mutex after computing the hash.
    Lock lock(mutex_);
    return bucket.Lookup(str);
}


} // namespace Detail
} // namespace AF

