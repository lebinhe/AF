#ifndef AF_FRAMEWORK_H
#define AF_FRAMEWORK_H

#include <new>

#include "AF/address.h"
#include "AF/align.h"
#include "AF/allocator_interface.h"
#include "AF/allocator_manager.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/allocators/caching_allocator.h"

#include "AF/detail/directory/directory.h"
#include "AF/detail/directory/entry.h"

#include "AF/detail/handlers/default_fallback_handler.h"
#include "AF/detail/handlers/fallback_handler_collection.h"

#include "AF/detail/mailboxes/mailbox.h"

#include "AF/detail/messages/message_creator.h"

#include "AF/detail/scheduler/counting.h"
#include "AF/detail/scheduler/mailbox_context.h"
#include "AF/detail/scheduler/scheduler_interface.h"

#include "AF/detail/strings/string.h"
#include "AF/detail/strings/string_pool.h"

#include "AF/detail/threading/atomic.h"
#include "AF/detail/threading/spin_lock.h"


namespace AF
{


class Actor;


class Framework : public Detail::Entry::Entity {
public:

    friend class Actor;

    struct Parameters {
        inline explicit Parameters(
            const uint32_t thread_count = 16) 
          : thread_count_(thread_count) {
        }

        uint32_t thread_count_;   // The initial number of worker threads to create within the framework.
    };

    inline explicit Framework(const uint32_t thread_count);

    inline explicit Framework(const Parameters &params = Parameters());

    inline ~Framework();

    template <typename ValueType>
    inline bool Send(const ValueType &value, const Address &from, const Address &address);

    inline void SetMaxThreads(const uint32_t count);

    inline void SetMinThreads(const uint32_t count);

    inline uint32_t GetMaxThreads() const;

    inline uint32_t GetMinThreads() const;

    inline uint32_t GetNumThreads() const;

    inline uint32_t GetPeakThreads() const;

    inline uint32_t GetNumCounters() const;

    inline const char *GetCounterName(const uint32_t counter) const;

    inline void ResetCounters();

    inline uint32_t GetCounterValue(const uint32_t counter) const;

    inline uint32_t GetPerThreadCounterValues(
        const uint32_t counter,
        uint32_t *const per_thread_counts,
        const uint32_t max_counts) const;

    template <typename ObjectType>
    inline bool SetFallbackHandler(
        ObjectType *const actor,
        void (ObjectType::*handler)(const Address from));

    template <typename ObjectType>
    inline bool SetFallbackHandler(
        ObjectType *const actor,
        void (ObjectType::*handler)(const void *const data, const uint32_t size, const Address from));

    static bool DeliverWithinLocalProcess(
        Detail::MessageInterface *const message,
        const Detail::Index &index);

private:

    struct MessageCacheTraits {
        typedef Detail::SpinLock LockType;

        struct AF_PREALIGN(AF_CACHELINE_ALIGNMENT) AlignType
        {
        } AF_POSTALIGN(AF_CACHELINE_ALIGNMENT);

        static const uint32_t MAX_POOLS = 8;
        static const uint32_t MAX_BLOCKS = 16;
    };

    typedef Detail::CachingAllocator<MessageCacheTraits> MessageCache;

    Framework(const Framework &other);
    Framework &operator=(const Framework &other);

    void Initialize();

    void Release();

    Detail::SchedulerInterface *CreateScheduler();

    void DestroyScheduler(Detail::SchedulerInterface *const scheduler);

    void RegisterActor(Actor *const actor, const char *const name = 0);

    void DeregisterActor(Actor *const actor);

    inline bool SendInternal(
        Detail::MailboxContext *const mailbox_context,
        Detail::MessageInterface *const message,
        Address address);

    inline bool FrameworkReceive(
        Detail::MessageInterface *const message,
        const Address &address);

    inline Detail::MailboxContext *GetMailboxContext();

    Detail::StringPool::Ref string_pool_ref_;                 // Ensures that the StringPool is created.
    const Parameters params_;                                 // Copy of parameters struct provided on construction.
    uint32_t index_;                                          // Non-zero index of this framework, unique within the local process.
    Detail::String name_;                                     // Name of this framework.
    Detail::Directory<Detail::Mailbox> mailboxes_;            // Per-framework mailbox array.
    Detail::FallbackHandlerCollection fallback_handlers_;     // Registered message handlers run for unhandled messages.
    Detail::DefaultFallbackHandler default_fallback_handler_; // Default handler for unhandled messages.
    MessageCache message_allocator_;                          // Thread-safe per-framework cache of message memory blocks.
    Detail::MailboxContext shared_mailbox_context_;           // Shared per-framework mailbox context.
    Detail::SchedulerInterface *scheduler_;                   // Pointer to owned scheduler implementation.
};


inline Framework::Framework(const uint32_t thread_count) 
  : string_pool_ref_(),
    params_(thread_count),
    index_(0),
    name_(),
    mailboxes_(),
    fallback_handlers_(),
    default_fallback_handler_(),
    message_allocator_(AllocatorManager::GetCache()),
    shared_mailbox_context_(),
    scheduler_(0) {

    Initialize();
}

inline Framework::Framework(const Parameters &params) 
  : string_pool_ref_(),
    params_(params),
    index_(0),
    name_(),
    mailboxes_(),
    fallback_handlers_(),
    default_fallback_handler_(),
    message_allocator_(AllocatorManager::GetCache()),
    shared_mailbox_context_(),
    scheduler_(0) {

    Initialize();
}

inline Framework::~Framework() {
    Release();
}

template <typename ValueType>
AF_FORCEINLINE bool Framework::Send(const ValueType &value, const Address &from, const Address &address) {
    // We use a thread-safe per-framework message cache to allocate messages sent from non-actor code.
    AllocatorInterface *const message_allocator(&message_allocator_);

    // Allocate a message. It'll be deleted by the worker thread that handles it.
    Detail::MessageInterface *const message(Detail::MessageCreator::Create(message_allocator, value, from));
    if (message == 0) {
        return false;
    }

    // Call the message sending implementation using the processor context of the framework.
    // When messages are sent using Framework::Send there's no obvious worker thread.
    return SendInternal(
        &shared_mailbox_context_,
        message,
        address);
}

AF_FORCEINLINE void Framework::SetMaxThreads(const uint32_t count) {
    scheduler_->SetMaxThreads(count);
}

AF_FORCEINLINE void Framework::SetMinThreads(const uint32_t count) {
    scheduler_->SetMinThreads(count);
}

AF_FORCEINLINE uint32_t Framework::GetMaxThreads() const {
    return scheduler_->GetMaxThreads();
}

AF_FORCEINLINE uint32_t Framework::GetMinThreads() const {
    return scheduler_->GetMinThreads();
}

AF_FORCEINLINE uint32_t Framework::GetNumThreads() const {
    return scheduler_->GetNumThreads();
}

AF_FORCEINLINE uint32_t Framework::GetPeakThreads() const {
    return scheduler_->GetPeakThreads();
}

AF_FORCEINLINE uint32_t Framework::GetNumCounters() const {
#if aF_ENABLE_COUNTERS
    return Detail::MAX_COUNTERS;
#else
    return 0;
#endif
}

AF_FORCEINLINE const char *Framework::GetCounterName(const uint32_t counter) const {
    if (counter < Detail::MAX_COUNTERS) {
#if AF_ENABLE_COUNTERS
        switch (counter) {
            case Detail::COUNTER_MESSAGES_PROCESSED:        return "messages processed";
            case Detail::COUNTER_YIELDS:                    return "thread yields";
            case Detail::COUNTER_LOCAL_PUSHES:              return "mailboxes pushed to thread-local message queue";
            case Detail::COUNTER_SHARED_PUSHES:             return "mailboxes pushed to per-framework message queue";
            case Detail::COUNTER_MAILBOX_QUEUE_MAX:         return "maximum size of mailbox queue";
            case Detail::COUNTER_QUEUE_LATENCY_LOCAL_MIN:   return "minimum observed latency of thread-local queue";
            case Detail::COUNTER_QUEUE_LATENCY_LOCAL_MAX:   return "maximum observed latency of thread-local queue";
            case Detail::COUNTER_QUEUE_LATENCY_SHARED_MIN:  return "minimum observed latency of per-framework queue";
            case Detail::COUNTER_QUEUE_LATENCY_SHARED_MAX:  return "maximum observed latency of per-framework queue";
            default: return "unknown";
        }
#endif
    }

    return "unknown";
}

AF_FORCEINLINE void Framework::ResetCounters() {
#if AF_ENABLE_COUNTERS
    scheduler_->ResetCounters();
#endif
}

AF_FORCEINLINE uint32_t Framework::GetCounterValue(const uint32_t counter) const {
    if (counter < Detail::MAX_COUNTERS) {
#if AF_ENABLE_COUNTERS
        return scheduler_->GetCounterValue(counter);
#endif
    }

    return 0;
}

AF_FORCEINLINE uint32_t Framework::GetPerThreadCounterValues(
    const uint32_t counter,
    uint32_t *const per_thread_counts,
    const uint32_t max_counts) const {
    if (counter < Detail::MAX_COUNTERS && per_thread_counts && max_counts) {
#if AF_ENABLE_COUNTERS
        return scheduler_->GetPerThreadCounterValues(counter, per_thread_counts, max_counts);
#endif
    }

    return 0;
}

AF_FORCEINLINE bool Framework::SendInternal(
    Detail::MailboxContext *const mailbox_context,
    Detail::MessageInterface *const message,
    Address address) {
    // The address should have been resolved to a non-zero local index.
    AF_ASSERT(address.index_.uint32_);

    // Is the addressed entity in the local framework?
    if (address.index_.componets_.framework_ == index_) {
        // Message is addressed to an actor in the sending framework.
        // Get a reference to the destination mailbox.
        Detail::Mailbox &mailbox(mailboxes_.GetEntry(address.index_.componets_.index_));

        // Push the message into the mailbox and schedule the mailbox for processing
        // if it was previously empty, so won't already be scheduled.
        // The message will be destroyed by the worker thread that does the processing,
        // even if it turns out that no actor is registered with the mailbox.
        mailbox.Lock();

        const bool schedule(mailbox.Empty());
        mailbox.Push(message);

        if (schedule) {
            scheduler_->Schedule(mailbox_context, &mailbox);
        }

        mailbox.Unlock();

        return true;
    }

    // Message is addressed to a mailbox in the local process but not in the
    // sending Framework. In this less common case we pay the hit of an extra call.
    if (DeliverWithinLocalProcess(message, address.index_)) {
        return true;
    }

    // Destroy the undelivered message.
    fallback_handlers_.Handle(message);
    Detail::MessageCreator::Destroy(&message_allocator_, message);

    return false;
}

AF_FORCEINLINE bool Framework::FrameworkReceive(
    Detail::MessageInterface *const message,
    const Address &address) {
    // Call the generic message sending function.
    // We use our own local context here because we're receiving the message.
    return SendInternal(
        &shared_mailbox_context_,
        message,
        address);
}

AF_FORCEINLINE Detail::MailboxContext *Framework::GetMailboxContext() {
    return &shared_mailbox_context_;
}

template <typename ObjectType>
inline bool Framework::SetFallbackHandler(
    ObjectType *const handler_object,
    void (ObjectType::*handler)(const Address from)) {
    return fallback_handlers_.Set(handler_object, handler);
}

template <typename ObjectType>
inline bool Framework::SetFallbackHandler(
    ObjectType *const handler_object,
    void (ObjectType::*handler)(const void *const data, const uint32_t size, const Address from)) {
    return fallback_handlers_.Set(handler_object, handler);
}


} // namespace AF


#endif // AF_FRAMEWORK_H
