#ifndef AF_DETAIL_SCHEDULER_MAILBOXCONTEXT_H
#define AF_DETAIL_SCHEDULER_MAILBOXCONTEXT_H


#include "AF/basic_types.h"
#include "AF/defines.h"
#include "AF/allocator_interface.h"

#include "AF/detail/mailboxes/mailbox.h"

#include "AF/detail/handlers/fallback_handler_collection.h"

#include "AF/detail/scheduler/scheduler_interface.h"


namespace AF
{
namespace Detail
{

/*
 * Context structure holding data used by a worker thread to process mailboxes.
 * 
 * The members of a single context are all accessed only by one worker thread
 * so we don't need to worry about shared writes, including false sharing.
 */
class MailboxContext {
public:
    inline MailboxContext() 
      : scheduler_(0),
        queue_context_(0),
        fallback_handlers_(0),
        message_allocator_(0),
        mailbox_(0),
        predicted_send_count_(0),
        send_count_(0) {
    }

    SchedulerInterface *scheduler_;                      // Pointer to the associated scheduler.
    void *queue_context_;                                // Pointer to the associated queue context.
    FallbackHandlerCollection *fallback_handlers_;       // Pointer to fallback handlers for undelivered messages.
    AllocatorInterface *message_allocator_;              // Pointer to message memory block allocator.
    Mailbox *mailbox_;                                   // Pointer to the mailbox that is being processed.
    uint32_t predicted_send_count_;                      // Number of messages predicted to be sent by the handler.
    uint32_t send_count_;                                // Messages sent so far by the handler being executed.

private:
    MailboxContext(const MailboxContext &other);
    MailboxContext &operator=(const MailboxContext &other);
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_SCHEDULER_MAILBOXCONTEXT_H
