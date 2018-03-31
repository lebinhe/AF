#ifndef AF_DETAIL_SCHEDULER_SCHEDULER_H
#define AF_DETAIL_SCHEDULER_SCHEDULER_H


#include "AF/align.h"
#include "AF/allocator_interface.h"
#include "AF/allocator_manager.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/containers/list.h"

#include "AF/detail/directory/directory.h"

#include "AF/detail/handlers/fallback_handler_collection.h"

#include "AF/detail/mailboxes/mailbox.h"

#include "AF/detail/threading/atomic.h"
#include "AF/detail/threading/condition.h"
#include "AF/detail/threading/mutex.h"
#include "AF/detail/threading/thread.h"

#include "AF/detail/scheduler/mailbox_context.h"
#include "AF/detail/scheduler/mailbox_processor.h"
#include "AF/detail/scheduler/thread_pool.h"
#include "AF/detail/scheduler/worker_context.h"
#include "AF/detail/scheduler/scheduler_hints.h"
#include "AF/detail/scheduler/scheduler_interface.h"

#include <new>


namespace AF
{
namespace Detail
{

/*
 * Mailbox scheduler.
 */
template <class QueueType>
class Scheduler : public SchedulerInterface {
public:
    inline explicit Scheduler(
        Directory<Mailbox> *const mailboxes,
        FallbackHandlerCollection *const fallback_handlers,
        AllocatorInterface *const message_allocator,
        MailboxContext *const shared_mailbox_context);

    inline virtual ~Scheduler();

    inline virtual void Initialize(const uint32_t thread_count);

    /*
     * Tears down the scheduler prior to destruction.
     */
    inline virtual void Release();

    /*
     * Notifies the scheduler that a worker thread is about to start executing a message handler.
     */
    inline virtual void BeginHandler(MailboxContext *const mailbox_context, MessageHandlerInterface *const message_handler);

    /*
     * Notifies the scheduler that a worker thread has finished executing a message handler.
     */
    inline virtual void EndHandler(MailboxContext *const mailbox_context, MessageHandlerInterface *const message_handler);

    /*
     * Schedules for processing a mailbox that has received a message.
     */
    inline virtual void Schedule(MailboxContext *const mailbox_context, Mailbox *const mailbox);

    inline virtual void SetMaxThreads(const uint32_t count);
    inline virtual void SetMinThreads(const uint32_t count);
    inline virtual uint32_t GetMaxThreads() const;
    inline virtual uint32_t GetMinThreads() const;
    inline virtual uint32_t GetNumThreads() const;
    inline virtual uint32_t GetPeakThreads() const;
    inline virtual void ResetCounters();
    inline virtual uint32_t GetCounterValue(const uint32_t counter) const;

    inline virtual uint32_t GetPerThreadCounterValues(
        const uint32_t counter,
        uint32_t *const per_thread_counts,
        const uint32_t max_counts) const;

private:
    typedef typename QueueType::ContextType QueueContext;
    typedef Detail::ThreadPool<QueueType, WorkerContext, MailboxProcessor> ThreadPool;
    typedef typename ThreadPool::ThreadContext ThreadContext;
    typedef List<ThreadContext> ContextList;

    Scheduler(const Scheduler &other);
    Scheduler &operator=(const Scheduler &other);

    /*
     * Checks whether all work queues are empty.
     */
    inline bool QueuesEmpty() const;

    /*
     * Static entry point function for the manager thread.
     * This is a static function that calls the real entry point member function.
     */
    inline static void ManagerThreadEntryPoint(void *const context);

    /*
     * Entry point member function for the manager thread.
     */
    inline void ManagerThreadProc();

    // Referenced external objects.
    Directory<Mailbox> *mailboxes_;                     // Pointer to external mailbox array.
    FallbackHandlerCollection *fallback_handlers_;      // Pointer to external fallback message handler collection.
    AllocatorInterface *message_allocator_;             // Pointer to external message memory block allocator.
    MailboxContext *shared_mailbox_context_;            // Pointer to external mailbox context shared by all worker threads.

    QueueContext shared_queue_context_;                 // Per-framework queue context shared by all worker threads.
    QueueType queue_;                                   // Instantiation of the work queue implementation.

    // Manager thread state.
    Thread manager_thread_;                             // Dynamically creates and destroys the worker threads.
    bool running_;                                      // Flag used to terminate the manager thread.
    Atomic::UInt32 target_thread_count_;                // Desired number of worker threads.
    Atomic::UInt32 peak_thread_count_;                  // Peak number of worker threads.
    Atomic::UInt32 thread_count_;                       // Actual number of worker threads.
    ContextList thread_contexts_;                       // List of worker thread context objects.
    mutable Mutex thread_context_lock_;                 // Protects the thread context list.
};


template <class QueueType>
inline Scheduler<QueueType>::Scheduler(
    Directory<Mailbox> *const mailboxes,
    FallbackHandlerCollection *const fallback_handlers,
    AllocatorInterface *const message_allocator,
    MailboxContext *const shared_mailbox_context)
  : mailboxes_(mailboxes),
    fallback_handlers_(fallback_handlers),
    message_allocator_(message_allocator),
    shared_mailbox_context_(shared_mailbox_context),
    shared_queue_context_(),
    queue_(),
    manager_thread_(),
    running_(false),
    target_thread_count_(0),
    peak_thread_count_(0),
    thread_count_(0),
    thread_contexts_(),
    thread_context_lock_() {
}

template <class QueueType>
inline Scheduler<QueueType>::~Scheduler() {
}

template <class QueueType>
inline void Scheduler<QueueType>::Initialize(const uint32_t thread_count) {
    // Set up the shared mailbox context.
    // This context is used by worker threads when a per-thread context isn't available.
    // The mailbox context holds pointers to the scheduler and associated queue context.
    // These are used to push mailboxes that still need further processing.
    shared_mailbox_context_->message_allocator_ = message_allocator_;
    shared_mailbox_context_->fallback_handlers_ = fallback_handlers_;
    shared_mailbox_context_->scheduler_ = this;
    shared_mailbox_context_->queue_context_ = &shared_queue_context_;

    queue_.InitializeSharedContext(&shared_queue_context_);

    // Set the initial thread count and affinity masks.
    thread_count_.Store(0);
    target_thread_count_.Store(thread_count);

    // Start the manager thread.
    running_ = true;
    manager_thread_.Start(ManagerThreadEntryPoint, this);

    // Wait for the manager thread to start all the worker threads.
    uint32_t backoff(0);
    while (thread_count_.Load() < target_thread_count_.Load()) {
        Utils::Backoff(backoff);
    }
}

template <class QueueType>
inline void Scheduler<QueueType>::Release() {
    // Wait for the work queue to drain, to avoid memory leaks.
    uint32_t backoff(0);
    while (!QueuesEmpty()) {
        Utils::Backoff(backoff);
    }

    // Reset the target thread count so the manager thread will kill all the threads.
    target_thread_count_.Store(0);

    // Wait for all the running threads to be stopped.
    backoff = 0;
    while (thread_count_.Load() > 0) {
        // Pulse any threads that are waiting so they can terminate.
        queue_.WakeAll();
        Utils::Backoff(backoff);
    }

    // Kill the manager thread and wait for it to terminate.
    running_ = false;
    manager_thread_.Join();

    queue_.ReleaseSharedContext(&shared_queue_context_);
}

template <class QueueType>
inline void Scheduler<QueueType>::BeginHandler(MailboxContext *const mailbox_context, MessageHandlerInterface *const message_handler) {
    // Store the last send count for this handler in the context so it's available to the scheduler.
    // Reset the message send count in the context and start counting sends for this handler.
    mailbox_context->predicted_send_count_ = message_handler->GetPredictedSendCount();
    mailbox_context->send_count_ = 0;
}

template <class QueueType>
inline void Scheduler<QueueType>::EndHandler(MailboxContext *const mailbox_context, MessageHandlerInterface *const message_handler) {
    // Update the cached message send count for this handler.
    // These counts are used to predict which of a handler's message sends will be its last.
    message_handler->ReportSendCount(mailbox_context->send_count_);
}

template <class QueueType>
inline void Scheduler<QueueType>::Schedule(MailboxContext *const mailbox_context, Mailbox *const mailbox) {
    QueueContext *const queue_context(reinterpret_cast<QueueContext *>(mailbox_context->queue_context_));
    Mailbox *const sending_mailbox(mailbox_context->mailbox_);

    // Build a hint structure to pass to the queuing policy.
    SchedulerHints hints;

    // Whether the mailbox is being scheduled because it received a message.
    // The other possibility is that it's the sending mailbox being rescheduled.
    hints.send_ = (sending_mailbox != mailbox);

    // The predicted number of messages sent by the message handler currently being invoked (if any).
    hints.predicted_send_count_ = mailbox_context->predicted_send_count_;

    // The actual number of messages sent so far by the handler being invoked (if any).
    hints.send_index_ = mailbox_context->send_count_;

    // The number of messages queued in the sending mailbox, for sends.
    // The count includes the message that is currently being processed.
    hints.message_count_ = 0;
    if (sending_mailbox) {
        hints.message_count_ = sending_mailbox->Count();
    }

    queue_.Push(queue_context, mailbox, hints);

    // We remember the number of messages each message handler sends, so we can
    // guess whether a given send will be the last next time it's executed.
    ++mailbox_context->send_count_;
}

template <class QueueType>
inline void Scheduler<QueueType>::SetMaxThreads(const uint32_t count) {
    if (target_thread_count_.Load() > count) {
        target_thread_count_.Store(count);
    }
}

template <class QueueType>
inline void Scheduler<QueueType>::SetMinThreads(const uint32_t count) {
    if (target_thread_count_.Load() < count) {
        target_thread_count_.Store(count);
    }
}

template <class QueueType>
inline uint32_t Scheduler<QueueType>::GetMaxThreads() const {
    return target_thread_count_.Load();
}

template <class QueueType>
inline uint32_t Scheduler<QueueType>::GetMinThreads() const {
    return target_thread_count_.Load();
}

template <class QueueType>
inline uint32_t Scheduler<QueueType>::GetNumThreads() const {
    return thread_count_.Load();
}

template <class QueueType>
inline uint32_t Scheduler<QueueType>::GetPeakThreads() const {
    return peak_thread_count_.Load();
}

template <class QueueType>
inline bool Scheduler<QueueType>::QueuesEmpty() const {
    // Check the shared queue context.
    if (!queue_.Empty(&shared_queue_context_)) {
        return false;
    }

    bool empty(true);

    thread_context_lock_.Lock();
    
    // Check the worker thread queue contexts.
    typename ContextList::Iterator contexts(thread_contexts_.GetIterator());
    while (contexts.Next()) {
        ThreadContext *const thread_context(contexts.Get());
        if (!queue_.Empty(&thread_context->queue_context_)) {
            empty = false;
            break;
        }
    }

    thread_context_lock_.Unlock();

    return empty;
}

template <class QueueType>
inline void Scheduler<QueueType>::ResetCounters() {
    // Reset the counters in the shared thread context.
    for (uint32_t counter = 0; counter < (uint32_t) MAX_COUNTERS; ++counter) {
        queue_.ResetCounter(&shared_queue_context_, counter);
    }

    thread_context_lock_.Lock();

    // Reset the counters in all worker thread contexts.
    typename ContextList::Iterator contexts(thread_contexts_.GetIterator());
    while (contexts.Next()) {
        ThreadContext *const thread_context(contexts.Get());
        for (uint32_t counter = 0; counter < (uint32_t) MAX_COUNTERS; ++counter) {
            queue_.ResetCounter(&thread_context->queue_context_, counter);
        }
    }

    thread_context_lock_.Unlock();
}

template <class QueueType>
inline uint32_t Scheduler<QueueType>::GetCounterValue(const uint32_t counter) const {
    // Read the counter value in the shared context.
    uint32_t accumulator(queue_.GetCounterValue(&shared_queue_context_, counter));

    thread_context_lock_.Lock();

    // Accumulate the counter values from all thread contexts.
    typename ContextList::Iterator contexts(thread_contexts_.GetIterator());
    while (contexts.Next()) {
        ThreadContext *const thread_context(contexts.Get());

        queue_.AccumulateCounterValue(
            &thread_context->queue_context_,
            counter,
            accumulator);
    }

    thread_context_lock_.Unlock();

    return accumulator;
}

template <class QueueType>
inline uint32_t Scheduler<QueueType>::GetPerThreadCounterValues(
    const uint32_t counter,
    uint32_t *const per_thread_counts,
    const uint32_t max_counts) const {

    uint32_t item_count(0);

    // Read the counter value in the shared context first.
    per_thread_counts[item_count++] = queue_.GetCounterValue(&shared_queue_context_, counter);

    thread_context_lock_.Lock();

    // Read the per-thread counter values into the provided array, skipping non-running contexts.
    typename ContextList::Iterator contexts(thread_contexts_.GetIterator());
    while (item_count < max_counts && contexts.Next()) {
        ThreadContext *const thread_context(contexts.Get());
        if (ThreadPool::IsRunning(thread_context)) {
            per_thread_counts[item_count++] = queue_.GetCounterValue(&thread_context->queue_context_, counter);
        }
    }

    thread_context_lock_.Unlock();

    return item_count;
}

template <class QueueType>
inline void Scheduler<QueueType>::ManagerThreadEntryPoint(void *const context) {
    // The static entry point function is passed the object pointer as context.
    Scheduler *const framework(reinterpret_cast<Scheduler *>(context));
    framework->ManagerThreadProc();
}

template <class QueueType>
inline void Scheduler<QueueType>::ManagerThreadProc() {
    AllocatorInterface *const allocator(AllocatorManager::GetCache());

    while (running_) {
        thread_context_lock_.Lock();

        // Re-start stopped worker threads while the thread count is too low.
        typename ContextList::Iterator contexts(thread_contexts_.GetIterator());
        while (thread_count_.Load() < target_thread_count_.Load() && contexts.Next()) {
            ThreadContext *const thread_context(contexts.Get());
            if (!ThreadPool::IsRunning(thread_context)) {
                if (!ThreadPool::StartThread(
                    thread_context)) {
                    break;
                }

                thread_count_.Increment();
            }
        }

        // Create new worker threads while the thread count is still too low.
        while (thread_count_.Load() < target_thread_count_.Load()) {
            // Create a thread context structure wrapping the worker context.
            void *const context_memory = allocator->AllocateAligned(sizeof(ThreadContext), AF_CACHELINE_ALIGNMENT);
            AF_ASSERT_MSG(context_memory, "Failed to allocate worker thread context");

            ThreadContext *const thread_context = new (context_memory) ThreadContext(&queue_);

            // Set up the mailbox context for the worker thread.
            // The mailbox context holds pointers to the scheduler and queue context.
            // These are used to push mailboxes that still need further processing.
            thread_context->user_context_.message_cache_.SetAllocator(message_allocator_);
            thread_context->user_context_.mailbox_context_.message_allocator_ = &thread_context->user_context_.message_cache_;
            thread_context->user_context_.mailbox_context_.fallback_handlers_ = fallback_handlers_;
            thread_context->user_context_.mailbox_context_.scheduler_ = this;
            thread_context->user_context_.mailbox_context_.queue_context_ = &thread_context->queue_context_;

            // Create a worker thread with the created context.
            if (!ThreadPool::CreateThread(thread_context)) {
                AF_FAIL_MSG("Failed to create worker thread");
            }

            // Start the thread on the given node and processors.
            if (!ThreadPool::StartThread(
                thread_context)) {
                AF_FAIL_MSG("Failed to start worker thread");
            }

            // Remember the context so we can reuse it and eventually destroy it.
            thread_contexts_.Insert(thread_context);

            // Track the peak thread count.
            thread_count_.Increment();
            if (thread_count_.Load() > peak_thread_count_.Load()) {
                peak_thread_count_.Store(thread_count_.Load());
            }
        }

        // Stop some running worker threads while the thread count is too high.
        contexts = thread_contexts_.GetIterator();
        while (thread_count_.Load() > target_thread_count_.Load() && contexts.Next()) {
            ThreadContext *const thread_context(contexts.Get());
            if (ThreadPool::IsRunning(thread_context)) {
                // Mark the thread as stopped and wake all the waiting threads.
                ThreadPool::StopThread(thread_context);
                queue_.WakeAll();

                // Wait for the stopped thread to actually terminate.
                ThreadPool::JoinThread(thread_context);

                thread_count_.Decrement();
            }
        }

        thread_context_lock_.Unlock();

        // The manager thread spends most of its time asleep.
        Utils::SleepThread(100);
    }

    // Free all the allocated thread context objects.
    while (!thread_contexts_.Empty()) {
        ThreadContext *const thread_context(thread_contexts_.Front());
        thread_contexts_.Remove(thread_context);

        // Wait for the thread to stop and then destroy it.
        ThreadPool::DestroyThread(thread_context);

        // Destruct and free the per-thread context.
        thread_context->~ThreadContext();
        allocator->FreeWithSize(thread_context, sizeof(ThreadContext));
    }
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_SCHEDULER_SCHEDULER_H
