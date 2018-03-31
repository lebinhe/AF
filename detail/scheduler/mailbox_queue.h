#ifndef AF_DETAIL_SCHEDULER_MAILBOXQUEUE_H
#define AF_DETAIL_SCHEDULER_MAILBOXQUEUE_H

#include "AF/align.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/containers/queue.h"

#include "AF/detail/mailboxes/mailbox.h"

#include "AF/detail/scheduler/counting.h"

#include "AF/detail/threading/atomic.h"
#include "AF/detail/threading/clock.h"

#include "AF/detail/scheduler/scheduler_hints.h"

#include "AF/detail/utils/utils.h"


namespace AF
{
namespace Detail
{

/*
 * Generic mailbox queue implementation with specialized per-thread local queues.
 */
template <class MonitorType>
class MailboxQueue {
public:
    // The item type which is queued by the queue.
    typedef Mailbox ItemType;

    // Context structure used to access the queue.
    class ContextType {
    public:
        friend class MailboxQueue;

        inline ContextType() 
          : running_(false),
            shared_(false),
            local_work_queue_(0) {
        }

    private:
        template <class ValueType>
        struct AF_PREALIGN(AF_CACHELINE_ALIGNMENT) Aligned {
            ValueType value_;

        } AF_POSTALIGN(AF_CACHELINE_ALIGNMENT);

        bool running_;                                      // Used to signal the thread to terminate.
        bool shared_;                                       // Indicates whether this is the 'shared' context.
        Mailbox *local_work_queue_;                         // Local thread-specific single-item work queue.
        typename MonitorType::Context monitor_context_;     // Per-thread monitor primitive context.
        Aligned<Atomic::UInt32> counters_[MAX_COUNTERS];    // Array of per-context event counters.
    };

    inline explicit MailboxQueue();

    // Initializes a user-allocated context as the 'shared' context common to all threads.
    inline void InitializeSharedContext(ContextType *const context);

    // Initializes a user-allocated context as the context associated with the calling thread.
    inline void InitializeWorkerContext(ContextType *const context);

    // Releases a previously initialized shared context.
    inline void ReleaseSharedContext(ContextType *const context);

    // Releases a previously initialized worker thread context.
    inline void ReleaseWorkerContext(ContextType *const context);

    // Resets to zero the given counter for the given thread context.
    inline void ResetCounter(ContextType *const context, const uint32_t counter) const;

    // Gets the value of the given counter for the given thread context.
    inline uint32_t GetCounterValue(const ContextType *const context, const uint32_t counter) const;

    // Accumulates the value of the given counter for the given thread context.
    inline void AccumulateCounterValue(
        const ContextType *const context,
        const uint32_t counter,
        uint32_t &accumulator) const;

    // Returns true if a call to Pop would return no mailbox, for the given context.
    inline bool Empty(const ContextType *const context) const;

    // Returns true if the thread with the given context is still enabled.
    inline bool Running(const ContextType *const context) const;

    // Wakes any worker threads which are blocked waiting for the queue to become non-empty.
    inline void WakeAll();

    // Pushes a mailbox into the queue, scheduling it for processing.
    inline void Push(ContextType *const context, Mailbox *mailbox, const SchedulerHints &hints);

    // Pops a previously pushed mailbox from the queue for processing.
    inline Mailbox *Pop(ContextType *const context);

private:
    MailboxQueue(const MailboxQueue &other);
    MailboxQueue &operator=(const MailboxQueue &other);

    inline static bool PreferLocalQueue(
        const ContextType *const context,
        const SchedulerHints &hints);

    mutable MonitorType monitor_;           // Synchronizes access to the shared queue.
    Queue<Mailbox> shared_work_queue_;      // Work queue shared by all the threads in a scheduler.
};


template <class MonitorType>
inline MailboxQueue<MonitorType>::MailboxQueue() 
  : monitor_() {
}

template <class MonitorType>
inline void MailboxQueue<MonitorType>::InitializeSharedContext(ContextType *const context) {
    context->shared_ = true;
}

template <class MonitorType>
inline void MailboxQueue<MonitorType>::InitializeWorkerContext(ContextType *const context) {
    // Only worker threads should call this method.
    context->shared_ = false;
    context->running_ = true;

    monitor_.InitializeWorkerContext(&context->monitor_context_);

    // The minimum counters need to be initialized to maxint.
    Counting::Reset(context->counters_[COUNTER_QUEUE_LATENCY_LOCAL_MIN].value_, COUNTER_QUEUE_LATENCY_LOCAL_MIN);
    Counting::Reset(context->counters_[COUNTER_QUEUE_LATENCY_SHARED_MIN].value_, COUNTER_QUEUE_LATENCY_SHARED_MIN);
}

template <class MonitorType>
inline void MailboxQueue<MonitorType>::ReleaseSharedContext(ContextType *const /*context*/) {
}

template <class MonitorType>
inline void MailboxQueue<MonitorType>::ReleaseWorkerContext(ContextType *const context) {
    typename MonitorType::LockType lock(monitor_);
    context->running_ = false;
}

template <class MonitorType>
inline void MailboxQueue<MonitorType>::ResetCounter(ContextType *const context, const uint32_t counter) const {
    Counting::Reset(context->counters_[counter].value_, counter);
}

template <class MonitorType>
AF_FORCEINLINE uint32_t MailboxQueue<MonitorType>::GetCounterValue(const ContextType *const context, const uint32_t counter) const {
    return Counting::Get(context->counters_[counter].value_);
}

template <class MonitorType>
AF_FORCEINLINE void MailboxQueue<MonitorType>::AccumulateCounterValue(
    const ContextType *const context,
    const uint32_t counter,
    uint32_t &accumulator) const {
    Counting::Accumulate(context->counters_[counter].value_, counter, accumulator);
}

template <class MonitorType>
AF_FORCEINLINE bool MailboxQueue<MonitorType>::Empty(const ContextType *const context) const {
    // Check the context's local queue.
    // If the provided context is the shared context then it doesn't have a local queue.
    if (!context->shared_ && context->local_work_queue_) {
        return false;
    }

    // Check the shared work queue.
    typename MonitorType::LockType lock(monitor_);
    return shared_work_queue_.Empty();
}

template <class MonitorType>
AF_FORCEINLINE bool MailboxQueue<MonitorType>::Running(const ContextType *const context) const {
    return context->running_;
}

template <class MonitorType>
AF_FORCEINLINE void MailboxQueue<MonitorType>::WakeAll() {
    monitor_.PulseAll();
}

template <class MonitorType>
AF_FORCEINLINE void MailboxQueue<MonitorType>::Push(
    ContextType *const context,
    Mailbox *mailbox,
    const SchedulerHints &hints) {
#if AF_ENABLE_COUNTERS
    
    // Timestamp the mailbox on entry.
    mailbox->Timestamp() = Clock::GetTicks();

#endif // AF_ENABLE_COUNTERS

    // Update the maximum mailbox queue length seen by this thread.
    Counting::Raise(context->counters_[COUNTER_MAILBOX_QUEUE_MAX].value_, mailbox->Count());

    // Choose whether to push the scheduled mailbox to the calling thread's
    // local queue (if the calling thread is a worker thread executing a message
    // handler) or the shared queue contended by all worker threads in the framework.
    if (PreferLocalQueue(context, hints)) {
        // If there's already a mailbox in the local queue then
        // swap it with the new mailbox. Effectively we promote the
        // previously pushed mailbox to the shared queue. This ensures we
        // never have more than one mailbox serialized on the local queue.
        // Promoting the earlier mailbox helps to promote fairness.
        // Also we now know that the earlier mailbox wasn't the last mailbox
        // messaged by this actor, whereas the new one might be. It's best
        // to push to the local queue only the last mailbox messaged by an
        // actor - ideally one messaged right at the end or 'tail' of the handler.
        // This constitutes a kind of tail recursion optimization.
        Mailbox *const previous(context->local_work_queue_);
        context->local_work_queue_ = mailbox;

        Counting::Increment(context->counters_[COUNTER_LOCAL_PUSHES].value_);

        if (previous == 0) {
            return;
        }

        mailbox = previous;
    }

    // Push the mailbox onto the shared work queue.
    // Because the shared queue is accessed by multiple threads we have to protect it.
    {
        typename MonitorType::LockType lock(monitor_);
        shared_work_queue_.Push(mailbox);
    }

    // Pulse the condition associated with the shared queue to wake a worker thread.
    // It's okay to release the lock before calling Pulse.
    monitor_.Pulse();
    Counting::Increment(context->counters_[COUNTER_SHARED_PUSHES].value_);
}

template <class MonitorType>
AF_FORCEINLINE Mailbox *MailboxQueue<MonitorType>::Pop(ContextType *const context) {
    Mailbox *mailbox(0);
    uint32_t counter_offset(0);

    // The shared context is never used to call Pop, only to Push
    // messages sent outside the context of a worker thread.
    AF_ASSERT(context->shared_ == false);

    // Try to pop a mailbox off the calling thread's local work queue.
    // We only check the shared queue once the local queue is empty.
    // Note that the local queue contains at most one item.
    if (context->local_work_queue_) {
        mailbox = context->local_work_queue_;
        context->local_work_queue_ = 0;
    } else {
        // Wait on the shared queue until we pop a mailbox from it.
        // Because the shared queue is accessed by multiple threads we have to protect it.
        typename MonitorType::LockType lock(monitor_);
        while (shared_work_queue_.Empty() && context->running_ == true) {
            Counting::Increment(context->counters_[COUNTER_YIELDS].value_);
            monitor_.Wait(&context->monitor_context_, lock);
        }

        if (!shared_work_queue_.Empty()) {
            mailbox = static_cast<Mailbox *>(shared_work_queue_.Pop());
            monitor_.ResetYield(&context->monitor_context_);
        }

        counter_offset = 2;
    }

    if (mailbox) {
        Counting::Increment(context->counters_[COUNTER_MESSAGES_PROCESSED].value_);

#if AF_ENABLE_COUNTERS

        // Compute the latency and update the maximum queue latency seen by this thread.
        const uint64_t timestamp(Clock::GetTicks());
        const uint64_t ticks(timestamp - mailbox->Timestamp());
        const uint64_t ticks_per_second(Clock::GetFrequency());
        const uint64_t usec(ticks * 1000000 / ticks_per_second);

        Atomic::UInt32 &max_counter(context->counters_[COUNTER_QUEUE_LATENCY_LOCAL_MAX + counter_offset].value_);
        Atomic::UInt32 &min_counter(context->counters_[COUNTER_QUEUE_LATENCY_LOCAL_MIN + counter_offset].value_);

        Counting::Raise(max_counter, static_cast<uint32_t>(usec));
        Counting::Lower(min_counter, static_cast<uint32_t>(usec));

#endif // AF_ENABLE_COUNTERS

    }

    return mailbox;
}

template <class MonitorType>
AF_FORCEINLINE bool MailboxQueue<MonitorType>::PreferLocalQueue(
    const ContextType *const context,
    const SchedulerHints &hints) {
    // The shared context doesn't have (or doesn't use) a local queue.
    if (context->shared_) {
        return false;
    }

    if (hints.send_) {
        // If this send isn't predicted to be the last then push it to the shared queue.
        if (hints.send_index_ + 1 < hints.predicted_send_count_) {
            return false;
        }

        // If the sending mailbox still has unprocessed messages then it will
        // be pushed to the local queue, so push this mailbox to the shared queue.
        if (hints.message_count_ > 1) {
            return false;
        }
    }

    return true;
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_SCHEDULER_MAILBOXQUEUE_H
