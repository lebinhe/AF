#ifndef AF_DETAIL_THREADPOOL_H
#define AF_DETAIL_THREADPOOL_H

#include "AF/align.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/containers/list.h"

#include "AF/detail/threading/thread.h"

#include "AF/detail/utils/utils.h"

#include <new>

namespace AF
{
namespace Detail
{

/*
 * A pool of worker threads that process a queue of work mailboxes.
 */
template <class QueueType, class ContextType, class ProcessorType>
class ThreadPool {
public:
    typedef typename QueueType::ItemType ItemType;
    typedef typename QueueType::ContextType QueueContext;

    class ThreadContext : public List<ThreadContext>::Node {
    public:
        // Constructor. Creates a ThreadContext wrapping a pointer to a per-thread scheduler object.
        inline explicit ThreadContext(QueueType *const queue) 
          : started_(false),
            thread_(0),
            queue_(queue),
            queue_context_(),
            user_context_() {
        }

        // Internal
        bool started_;                           // Indicates whether the thread has started.
        Thread *thread_;                         // Pointer to the thread object.

        // Client
        QueueType *const queue_;                // Pointer to the queue serviced by the worker threads.
        QueueContext queue_context_;            // Queue context associated with the worker thread.
        ContextType user_context_;              // User context data associated with the worker thread.

    private:
        ThreadContext(const ThreadContext &other);
        ThreadContext &operator=(const ThreadContext &other);
    };

    /*
     * Creates an additional worker thread for processing of work items.
     * The thread is created but not yet started - call StartThread for that.
     * threadContext: Pointer to a caller-allocated context object for the thread.
     */
    inline static bool CreateThread(ThreadContext *const thread_context);

    /*
     * Starts the given thread, which must have been created with CreateThread.
     * work_queue: Pointer to the shared work queue that the thread will service.
     */
    inline static bool StartThread(
        ThreadContext *const thread_context);

    /*
     * Stops the given thread, which must have been started with StartThread.
     */
    inline static bool StopThread(ThreadContext *const thread_context);

    /*
     * Waits for the given thread, which must have been stopped with StopThread, to terminate.
     */
    inline static bool JoinThread(ThreadContext *const thread_context);

    /*
     * Destroys the given thread, which must have been stopped with StopThread and JoinThread.
     */
    inline static bool DestroyThread(ThreadContext *const thread_context);

    /*
     * Returns true if the given thread has been started but not stopped.
     */
    inline static bool IsRunning(ThreadContext *const thread_context);

    /*
     * Returns true if the given thread has started.
     */
    inline static bool IsStarted(ThreadContext *const thread_context);

private:
    ThreadPool(const ThreadPool &other);
    ThreadPool &operator=(const ThreadPool &other);

    /*
     * Worker thread entry point function.
     * Only global (static) functions can be used as thread entry points.
     * context: Pointer to a context object that provides the context in which the thread is run.
     */
    inline static void ThreadEntryPoint(void *const context);
};


template <class QueueType, class ContextType, class ProcessorType>
inline bool ThreadPool<QueueType, ContextType, ProcessorType>::CreateThread(ThreadContext *const thread_context) {
    // Allocate a new thread, aligning the memory to a cache-line boundary to reduce false-sharing of cache-lines.
    void *const thread_memory = AllocatorManager::GetCache()->AllocateAligned(sizeof(Thread), AF_CACHELINE_ALIGNMENT);
    if (thread_memory == 0) {
        return false;
    }

    // Construct the thread object.
    Thread *const thread = new (thread_memory) Thread();

    // Set up the private part of the user-allocated context. We pass in a pointer to the
    // threadpool instance so we can use a member function as the entry point.
    thread_context->started_ = false;
    thread_context->thread_ = thread;
   
    return true;
}

template <class QueueType, class ContextType, class ProcessorType>
inline bool ThreadPool<QueueType, ContextType, ProcessorType>::StartThread(
    ThreadContext *const thread_context) {
    AF_ASSERT(thread_context->thread_);
    AF_ASSERT(thread_context->thread_->Running() == false);

    // Register the worker's queue context with the queue.
    thread_context->queue_->InitializeWorkerContext(&thread_context->queue_context_);

    // Start the thread, running it via a static (non-member function) entry point that wraps the real member function.
    thread_context->thread_->Start(ThreadEntryPoint, thread_context);

    return true;
}

template <class QueueType, class ContextType, class ProcessorType>
inline bool ThreadPool<QueueType, ContextType, ProcessorType>::StopThread(ThreadContext *const thread_context) {
    AF_ASSERT(thread_context->thread_);
    AF_ASSERT(thread_context->thread_->Running());

    // Deregister the worker's queue context.
    // Typically this instructs the thread to terminate.
    thread_context->queue_->ReleaseWorkerContext(&thread_context->queue_context_);

    return true;
}

template <class QueueType, class ContextType, class ProcessorType>
inline bool ThreadPool<QueueType, ContextType, ProcessorType>::JoinThread(ThreadContext *const thread_context) {
    AF_ASSERT(thread_context->thread_);
    AF_ASSERT(thread_context->thread_->Running());

    // Wait for the thread to finish.
    thread_context->thread_->Join();

    return true;
}

template <class QueueType, class ContextType, class ProcessorType>
inline bool ThreadPool<QueueType, ContextType, ProcessorType>::DestroyThread(ThreadContext *const thread_context) {
    AF_ASSERT(thread_context->thread_);
    AF_ASSERT(thread_context->thread_->Running() == false);

    // Destruct the thread object explicitly since we constructed using placement new.
    thread_context->thread_->~Thread();

    // Free the memory for the thread.
    AllocatorManager::GetCache()->FreeWithSize(thread_context->thread_, sizeof(Thread));
    thread_context->thread_ = 0;

    return true;
}

template <class QueueType, class ContextType, class ProcessorType>
AF_FORCEINLINE bool ThreadPool<QueueType, ContextType, ProcessorType>::IsRunning(ThreadContext *const thread_context) {
    return thread_context->queue_->Running(&thread_context->queue_context_);
}

template <class QueueType, class ContextType, class ProcessorType>
AF_FORCEINLINE bool ThreadPool<QueueType, ContextType, ProcessorType>::IsStarted(ThreadContext *const thread_context) {
    return thread_context->started_;
}

template <class QueueType, class ContextType, class ProcessorType>
inline void ThreadPool<QueueType, ContextType, ProcessorType>::ThreadEntryPoint(void *const context) {
    // The static entry point function is provided with a pointer to a context structure.
    // The context structure is specific to this worker thread.
    ThreadContext *const thread_context(reinterpret_cast<ThreadContext *>(context));
    QueueType *const queue(thread_context->queue_);
    QueueContext *const queue_context(&thread_context->queue_context_);
    ContextType *const user_context(&thread_context->user_context_);

    // Mark the thread as started so the caller knows they can start issuing work.
    thread_context->started_ = true;

    // Process items until told to stop.
    while (queue->Running(queue_context)) {
        if (ItemType *const item = queue->Pop(queue_context)) {
            ProcessorType::Process(user_context, item);
        }
    }
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_THREADPOOL_H
