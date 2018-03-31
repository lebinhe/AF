#ifndef AF_DETAIL_MAILBOXES_MAILBOX_H
#define AF_DETAIL_MAILBOXES_MAILBOX_H


#include "AF/align.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"


#include "AF/detail/containers/queue.h"

#include "AF/detail/messages/message_interface.h"

#include "AF/detail/strings/string.h"

#include "AF/detail/threading/spin_lock.h"


namespace AF
{

class Actor;

namespace Detail
{

/*
 * An individual mailbox with a specific address.
 */
class AF_PREALIGN(AF_CACHELINE_ALIGNMENT) Mailbox : public Queue<Mailbox>::Node {
public:
    inline Mailbox();

    // Gets the string name of the mailbox.
    // The name is arbitrary and identifies the actor within the context of the whole system,
    inline String GetName() const;

    inline void SetName(const String &name);

    inline void Lock() const;

    inline void Unlock() const;

    inline bool Empty() const;

    inline void Push(MessageInterface *const message);

    inline MessageInterface *Front() const;

    inline MessageInterface *Pop();

    inline uint32_t Count() const;

    // Registers an actor with this mailbox.
    inline void RegisterActor(Actor *const actor);

    // Deregisters the actor registered with this mailbox.
    inline void DeregisterActor();

    inline Actor *GetActor() const;

    // Pins the mailbox, preventing the registered actor from being changed.
    inline void Pin();

    // Unpins the mailbox, allowed the registered actor to be changed.
    inline void Unpin();

    inline bool IsPinned() const;

    inline uint64_t &Timestamp();

    inline const uint64_t &Timestamp() const;

private:

    typedef Queue<MessageInterface> MessageQueue;

    MessageQueue queue_;                        // Queue of messages in this mailbox.
    String name_;                               // Name of this mailbox.
    Actor *actor_;                              // Pointer to the actor registered with this mailbox, if any.
    mutable SpinLock spin_lock_;                // Thread synchronization object protecting the mailbox.
    uint32_t message_count_;                    // Size of the message queue.
    uint32_t pin_count_;                        // Pinning a mailboxes prevents the actor from being deregistered.
    uint64_t timestamp_;                        // Used for measuring mailbox scheduling latencies.

} AF_POSTALIGN(AF_CACHELINE_ALIGNMENT);


inline Mailbox::Mailbox() 
  : queue_(),
    name_(),
    actor_(0),
    spin_lock_(),
    message_count_(0),
    pin_count_(0),
    timestamp_(0) {
}

AF_FORCEINLINE String Mailbox::GetName() const {
    return name_;
}

AF_FORCEINLINE void Mailbox::SetName(const String &name) {
    name_ = name;
}

AF_FORCEINLINE void Mailbox::Lock() const {
    spin_lock_.Lock();
}

AF_FORCEINLINE void Mailbox::Unlock() const {
    spin_lock_.Unlock();
}

AF_FORCEINLINE bool Mailbox::Empty() const {
    return queue_.Empty();
}

AF_FORCEINLINE void Mailbox::Push(MessageInterface *const message) {
    queue_.Push(message);
    ++message_count_;
}

AF_FORCEINLINE MessageInterface *Mailbox::Front() const {
    return queue_.Front();
}

AF_FORCEINLINE MessageInterface *Mailbox::Pop() {
    --message_count_;
    return queue_.Pop();
}

AF_FORCEINLINE uint32_t Mailbox::Count() const {
    return message_count_;
}

AF_FORCEINLINE void Mailbox::RegisterActor(Actor *const actor) {
    // Can't register actors while the mailbox is pinned.
    AF_ASSERT(pin_count_ == 0);
    AF_ASSERT(actor_ == 0);
    AF_ASSERT(actor);

    actor_ = actor;
}

AF_FORCEINLINE void Mailbox::DeregisterActor() {
    // Can't deregister actors while the mailbox is pinned.
    AF_ASSERT(pin_count_ == 0);
    AF_ASSERT(actor_ != 0);

    actor_ = 0;
}

AF_FORCEINLINE Actor *Mailbox::GetActor() const {
    return actor_;
}

AF_FORCEINLINE void Mailbox::Pin() {
    ++pin_count_;
}

AF_FORCEINLINE void Mailbox::Unpin() {
    AF_ASSERT(pin_count_ > 0);
    --pin_count_;
}

AF_FORCEINLINE bool Mailbox::IsPinned() const {
    return (pin_count_ > 0);
}

AF_FORCEINLINE uint64_t &Mailbox::Timestamp() {
    return timestamp_;
}

AF_FORCEINLINE const uint64_t &Mailbox::Timestamp() const {
    return timestamp_;
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_MAILBOXES_MAILBOX_H

