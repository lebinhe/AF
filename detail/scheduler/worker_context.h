#ifndef AF_DETAIL_SCHEDULER_WORKERCONTEXT_H
#define AF_DETAIL_SCHEDULER_WORKERCONTEXT_H


#include "AF/detail/allocators/caching_allocator.h"
#include "AF/detail/scheduler/mailbox_context.h"


namespace AF
{
namespace Detail
{

/*
 * Per-worker thread context structure holding data used by a worker thread.
 * This is a wrapper used to hold a couple of different pieces of context data.
 */
class WorkerContext {
public:
    inline WorkerContext() {
    }

    CachingAllocator<> message_cache_;       // Per-thread cache of message memory blocks.
    MailboxContext mailbox_context_;         // Per-thread context for mailbox processing.

private:
    WorkerContext(const WorkerContext &other);
    WorkerContext &operator=(const WorkerContext &other);
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_SCHEDULER_WORKERCONTEXT_H
