#ifndef AF_DETAIL_DIRECTORY_STATICDIRECTORY_H
#define AF_DETAIL_DIRECTORY_STATICDIRECTORY_H


#include "AF/allocator_interface.h"
#include "AF/allocator_manager.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/threading/mutex.h"

#include "AF/detail/directory/directory.h"
#include "AF/detail/directory/entry.h"

#include <new>


namespace AF
{
namespace Detail
{

/*
 * Static class template that manages a reference-counted directory singleton.
 */
template <class Entity>
class StaticDirectory {
public:
    /* 
     * Registers an entity and returns its allocated index.
     */
    inline static uint32_t Register(Entry::Entity *const entity);

    /*
     * Deregisters a previously registered entity.
     */
    inline static void Deregister(const uint32_t index);

    inline static Entry &GetEntry(const uint32_t index);

private:
    typedef Directory<Entry> DirectoryType;

    static DirectoryType *directory_;          // Pointer to the allocated instance.
    static Mutex mutex_;                       // Synchronization object protecting access.
    static uint32_t reference_count_;          // Counts the number of entities registered.
};


template <class Entity>
typename StaticDirectory<Entity>::DirectoryType *StaticDirectory<Entity>::directory_ = 0;

template <class Entity>
Mutex StaticDirectory<Entity>::mutex_;

template <class Entity>
uint32_t StaticDirectory<Entity>::reference_count_ = 0;


template <class Entity>
inline uint32_t StaticDirectory<Entity>::Register(Entry::Entity *const entity) {
    mutex_.Lock();

    // Create the singleton instance if this is the first reference.
    if (reference_count_++ == 0) {
        AllocatorInterface *const allocator(AllocatorManager::GetCache());
        void *const memory(allocator->AllocateAligned(sizeof(DirectoryType), AF_CACHELINE_ALIGNMENT));

        if (memory == 0) {
            return 0;
        }

        directory_ = new (memory) DirectoryType();
    }

    AF_ASSERT(directory_);

    const uint32_t index(directory_->Allocate());

    // Set up the entry.
    if (index) {
        Entry &entry(directory_->GetEntry(index));
        entry.Lock();
        entry.SetEntity(entity);
        entry.Unlock();
    }

    mutex_.Unlock();

    return index;
}

template <class Entity>
inline void StaticDirectory<Entity>::Deregister(const uint32_t index) {
    mutex_.Lock();

    AF_ASSERT(directory_);
    AF_ASSERT(index);

    // Clear the entry.
    // If the entry is pinned then we have to wait for it to be unpinned.
    Entry &entry(directory_->GetEntry(index));

    bool deregistered(false);
    while (!deregistered) {
        entry.Lock();

        if (!entry.IsPinned()) {
            entry.Free();
            deregistered = true;
        }

        entry.Unlock();
    }

    // Destroy the singleton instance if this was the last reference.
    if (--reference_count_ == 0) {
        AllocatorInterface *const allocator(AllocatorManager::GetCache());
        directory_->~DirectoryType();
        allocator->FreeWithSize(directory_, sizeof(DirectoryType));
    }

    mutex_.Unlock();
}

template <class Entity>
AF_FORCEINLINE Entry &StaticDirectory<Entity>::GetEntry(const uint32_t index) {
    AF_ASSERT(directory_);
    AF_ASSERT(index);

    return directory_->GetEntry(index);
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_DIRECTORY_STATICDIRECTORY_H

