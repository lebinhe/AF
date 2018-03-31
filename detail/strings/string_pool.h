#ifndef AF_DETAIL_STRINGS_STRINGPOOL_H
#define AF_DETAIL_STRINGS_STRINGPOOL_H


#include "AF/align.h"
#include "AF/allocator_interface.h"
#include "AF/allocator_manager.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/containers/list.h"

#include "AF/detail/threading/mutex.h"

#include <new>
#include <string.h>
#include <stdlib.h>


namespace AF
{
namespace Detail
{


/*
 * Static class that manages a pool of unique strings.
 */
class StringPool {
public:
    /*
     * Holds a reference to the string pool, ensuring that it has been created.
     */
    class Ref {
    public:
        inline Ref() {
            StringPool::Reference();
        }

        inline ~Ref() {
            StringPool::Dereference();
        }
    };

    friend class Ref;

    // Gets the address of the pooled version of the given literal string.
    // The pooled version is created if it doesn't already exist.
    inline static const char *Get(const char *const str);

private:
    class Entry : public List<Entry>::Node {
    public:
        AF_FORCEINLINE static uint32_t GetSize(const char *const str) {
            const uint32_t length(static_cast<uint32_t>(strlen(str)));
            uint32_t length_with_null(length + 1);
            const uint32_t rounded_length(AF_ROUNDUP(length_with_null, 4));

            return sizeof(Entry) + rounded_length;
        }

        AF_FORCEINLINE static Entry *Initialize(void *const memory, const char *const str) {
            char *const buffer(reinterpret_cast<char *>(memory) + sizeof(Entry));
            strcpy(buffer, str);

            return new (memory) Entry(buffer);
        }

        AF_FORCEINLINE explicit Entry(const char *const val) : value_(val) {
        }

        AF_FORCEINLINE const char *Value() const {
            return value_;
        }

    private:
        const char *value_;
    };

    class Bucket {
    public:
        inline Bucket() {
        }

        inline ~Bucket() {
            AllocatorInterface *const allocator(AllocatorManager::GetCache());

            // Free all entries at end of day.
            while (!entries_.Empty()) {
                Entry *const entry(entries_.Front());
                entries_.Remove(entry);

                const uint32_t size(Entry::GetSize(entry->Value()));
                entry->~Entry();
                allocator->FreeWithSize(entry, size);
            }
        }

        AF_FORCEINLINE const char *Lookup(const char *const str) {
            AllocatorInterface *const allocator(AllocatorManager::GetCache());
            Entry *entry(0);

            // Search the bucket for an an existing entry for this string.
            List<Entry>::Iterator entries(entries_.GetIterator());
            while (entries.Next()) {
                Entry *const e(entries.Get());
                if (strcmp(e->Value(), str) == 0) {
                    entry = e;
                    break;
                }
            }

            if (entry == 0) {
                // Create a new entry.
                const uint32_t size(Entry::GetSize(str));
                void *const memory(allocator->Allocate(size));
                entry = Entry::Initialize(memory, str);
                entries_.Insert(entry);
            }

            return entry->Value();
        }

    private:
        List<Entry> entries_;
    };

    // References the string pool, creating the singleton instance if it doesn't already exist.
    static void Reference();

    // Releases a reference to the string pool, destroying the singleton instance if it is no longer referenced.
    static void Dereference();

    inline static uint32_t Hash(const char *const str);

    static StringPool *instance_;                   // Pointer to the singleton instance.
    static Mutex reference_mutex_;                  // Synchronization object protecting reference counting.
    static uint32_t reference_count_;               // Counts the number of references to the singleton.

    static const uint32_t BUCKET_COUNT = 128;

    StringPool();
    ~StringPool();

    // Finds the entry for a given string, and creates it if it doesn't exist yet.
    const char *Lookup(const char *const str);

    Mutex mutex_;
    Bucket buckets_[BUCKET_COUNT];
};


AF_FORCEINLINE const char *StringPool::Get(const char *const str) {
    AF_ASSERT(instance_);
    AF_ASSERT(str);

    return instance_->Lookup(str);
}

AF_FORCEINLINE uint32_t StringPool::Hash(const char *const str) {
    AF_ASSERT(str);

    // XOR the first n characters of the string together.
    const char *const end(str + 64);

    const char *ch(str);
    uint8_t hash(0);

    while (ch != end && *ch != '\0') {
        hash ^= static_cast<uint8_t>(*ch);
        ++ch;
    }

    // Zero the 8th bit since it's usually zero anyway in ASCII.
    // This gives an effective range of [0, 127].
    hash &= 127;

    return static_cast<uint32_t>(hash);
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_STRINGS_STRINGPOOL_H

