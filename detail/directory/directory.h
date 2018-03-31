#ifndef AF_DETAIL_DIRECTORY_DIRECTORY_H
#define AF_DETAIL_DIRECTORY_DIRECTORY_H


#include "AF/allocator_interface.h"
#include "AF/allocator_manager.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/threading/mutex.h"

#include <new>


namespace AF
{
namespace Detail
{

/*
 * A registry that maps unique indices to addressable entities.
 */
template <class EntryType>
class Directory {
public:
    Directory();

    ~Directory();

    /*
     * Finds and claims a free index for an entity.
     */
    uint32_t Allocate(uint32_t index = 0);

    /*
     * Gets a reference to the entry with the given index.
     */
    inline EntryType &GetEntry(const uint32_t index);

private:
    static const uint32_t ENTRIES_PER_PAGE = 1024;  // Number of entries in each allocated page (power of two!).
    static const uint32_t MAX_PAGES = 1024;         // Maximum number of allocated pages.

    struct Page {
        EntryType entries_[ENTRIES_PER_PAGE];       // Array of entries making up this page.
    };

    Directory(const Directory &other);
    Directory &operator=(const Directory &other);

    mutable Mutex mutex_;                           // Ensures thread-safe access to the instance data.
    uint32_t next_index_;                           // Auto-incremented index to use for next registered entity.
    Page *pages_[MAX_PAGES];                        // Pointers to allocated pages.
};


template <class EntryType>
inline Directory<EntryType>::Directory() 
  : mutex_(),
    next_index_(0) {
    // Clear the page table.
    for (uint32_t page = 0; page < MAX_PAGES; ++page) {
        pages_[page] = 0;
    }
}

template <class EntryType>
inline Directory<EntryType>::~Directory() {
    AllocatorInterface *const page_allocator(AllocatorManager::GetCache());

    // Free all pages that were allocated.
    for (uint32_t page = 0; page < MAX_PAGES; ++page) {
        if (pages_[page]) {
            // Destruct and free.
            pages_[page]->~Page();
            page_allocator->FreeWithSize(pages_[page], sizeof(Page));            
        }
    }
}

template <class EntryType>
inline uint32_t Directory<EntryType>::Allocate(uint32_t index) {
    mutex_.Lock();

    // Auto-allocate an index if none was specified.
    if (index == 0) {
        // TODO: Avoid in-use indices and re-use freed ones.
        // Skip index zero as it's reserved for use as the null address.
        if (++next_index_ == MAX_PAGES * ENTRIES_PER_PAGE) {
            next_index_ = 1;
        }

        index = next_index_;
    }

    // Allocate the page if it hasn't been allocated already.
    const uint32_t page(index / ENTRIES_PER_PAGE);
    if (pages_[page] == 0) {
        AllocatorInterface *const page_allocator(AllocatorManager::GetCache());
        void *const page_memory(page_allocator->AllocateAligned(sizeof(Page), AF_CACHELINE_ALIGNMENT));

        if (page_memory) {
            pages_[page] = new (page_memory) Page();
        } else {
            AF_FAIL_MSG("Out of memory");
        }
    }

    mutex_.Unlock();

    return index;
}

template <class EntryType>
AF_FORCEINLINE EntryType &Directory<EntryType>::GetEntry(const uint32_t index) {
    // Compute the page and offset.
    // TODO: Use a mask?
    const uint32_t page(index / ENTRIES_PER_PAGE);
    const uint32_t offset(index % ENTRIES_PER_PAGE);

    AF_ASSERT(page < MAX_PAGES);
    AF_ASSERT(offset < ENTRIES_PER_PAGE);
    AF_ASSERT(pages_[page]);

    return pages_[page]->entries_[offset];
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_DIRECTORY_DIRECTORY_H

