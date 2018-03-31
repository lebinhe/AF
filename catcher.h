#ifndef AF_CATCHER_H
#define AF_CATCHER_H


#include <new>

#include "AF/address.h"
#include "AF/align.h"
#include "AF/allocator_manager.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/containers/queue.h"

#include "AF/detail/threading/lock.h"
#include "AF/detail/threading/mutex.h"


namespace AF
{

template <class MessageType>
class Catcher {
public:
    inline Catcher();

    inline ~Catcher();

    inline bool Empty() const;

    inline void Push(const MessageType &message, const Address from);

    inline bool Front(MessageType &message, Address &from);

    inline bool Pop(MessageType &message, Address &from);

private:
    struct Entry : public Detail::Queue<Entry>::Node {
        inline Entry(const MessageType &message, const Address from) : message_(message), from_(from) {
        }

        MessageType message_;               // The queued message.
        Address from_;                      // The address of the sender.
    };

    mutable Detail::Mutex mutex_;           // Thread synchronization object.
    Detail::Queue<Entry> queue_;            // Queue of caught messages.
};


template <class MessageType>
inline Catcher<MessageType>::Catcher() : mutex_(), queue_() {
}

template <class MessageType>
inline Catcher<MessageType>::~Catcher() {
    AllocatorInterface *const allocator(AllocatorManager::GetCache());
    Detail::Lock lock(mutex_);

    // Free any left-over entries on the queue.
    while (!queue_.Empty()) {
        Entry *const entry(queue_.Pop());

        // Destruct the entry and free the memory.
        entry->~Entry();
        allocator->FreeWithSize(entry, sizeof(Entry));
    }
}

template <class MessageType>
AF_FORCEINLINE bool Catcher<MessageType>::Empty() const {
    Detail::Lock lock(mutex_);
    return queue_.Empty();
}

template <class MessageType>
inline void Catcher<MessageType>::Push(const MessageType &message, const Address from) {
    AllocatorInterface *const allocator(AllocatorManager::GetCache());

    // Allocate an entry to hold the copy of the message in the queue, with correct alignment.
    const uint32_t entry_size(static_cast<uint32_t>(sizeof(Entry)));
    const uint32_t entry_alignment(static_cast<uint32_t>(AF_ALIGNOF(Entry)));
    void *const memory(allocator->AllocateAligned(entry_size, entry_alignment));

    if (memory == 0) {
        return;
    }

    // Construct the entry in the allocated memory.
    Entry *const entry = new (memory) Entry(message, from);

    // Push the entry onto the queue, locking for thread-safety.
    Detail::Lock lock(mutex_);
    queue_.Push(entry);
}

template <class MessageType>
inline bool Catcher<MessageType>::Front(MessageType &message, Address &from) {
    Entry *entry(0);

    // Pop an entry off the queue, locking for thread-safety.
    mutex_.Lock();
    if (!queue_.Empty()) {
        entry = queue_.Front();
    }
    mutex_.Unlock();

    if (entry) {
        // Copy the data from the entry.
        message = entry->message_;
        from = entry->from_;

        return true;
    }

    return false;
}

template <class MessageType>
inline bool Catcher<MessageType>::Pop(MessageType &message, Address &from) {
    AllocatorInterface *const allocator(AllocatorManager::GetCache());
    Entry *entry(0);

    // Pop an entry off the queue, locking for thread-safety.
    mutex_.Lock();
    if (!queue_.Empty()) {
        entry = queue_.Pop();
    }
    mutex_.Unlock();

    if (entry) {
        // Copy the data from the entry.
        message = entry->message_;
        from = entry->from_;

        // Destruct the entry and free the memory.
        entry->~Entry();
        allocator->FreeWithSize(entry, sizeof(Entry));

        return true;
    }

    return false;
}


} // namespace AF


#endif // AF_CATCHER_H
