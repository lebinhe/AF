#ifndef AF_DETAIL_SCHEDULER_MAILBOXPROCESSOR_H
#define AF_DETAIL_SCHEDULER_MAILBOXPROCESSOR_H

#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/mailboxes/mailbox.h"
#include "AF/detail/scheduler/worker_context.h"

#include "AF/detail/handlers/fallback_handler_collection.h"

#include "AF/detail/messages/message_interface.h"
#include "AF/detail/messages/message_creator.h"


namespace AF
{

class Actor;

namespace Detail
{


/*
 * Processes mailboxes that have received messages.
 */
class MailboxProcessor {
public:
    inline static void Process(WorkerContext *const worker_context, Mailbox *const mailbox);
    
private:
    MailboxProcessor(const MailboxProcessor &other);
    MailboxProcessor &operator=(const MailboxProcessor &other);
};


AF_FORCEINLINE void MailboxProcessor::Process(WorkerContext *const worker_context, Mailbox *const mailbox) {
    // Load the context data from the worker thread's mailbox context.
    MailboxContext *const mailbox_context(&worker_context->mailbox_context_);
    FallbackHandlerCollection *const fallback_handlers(mailbox_context->fallback_handlers_);
    AllocatorInterface *const message_allocator(mailbox_context->message_allocator_);

    AF_ASSERT(fallback_handlers);
    AF_ASSERT(message_allocator);

    // Remember the mailbox we're processing in the context so we can query it.
    mailbox_context->mailbox_ = mailbox;

    // Pin the mailbox and get the registered actor and the first queued message.
    // At this point the mailbox shouldn't be enqueued in any other work items,
    // even if it contains more than one unprocessed message. This ensures that
    // each mailbox is only processed by one worker thread at a time.
    mailbox->Lock();
    mailbox->Pin();
    Actor *const actor(mailbox->GetActor());
    MessageInterface *const message(mailbox->Front());
    mailbox->Unlock();

    // If an actor is registered at the mailbox then process it.
    if (actor) {
        actor->ProcessMessage(mailbox_context, fallback_handlers, message);
    } else {
        fallback_handlers->Handle(message);
    }

    // Pop the message we just processed from the mailbox, then check whether the
    // mailbox is now empty, and reschedule the mailbox if it's not.
    // The locking of the mailbox here and in the main scheduling ensures that
    // mailboxes are always enqueued if they have unprocessed messages, but at most
    // once at any time.
    mailbox->Lock();
    mailbox->Unpin();
    mailbox->Pop();

    if (!mailbox->Empty()) {
        mailbox_context->scheduler_->Schedule(mailbox_context, mailbox);
    }

    mailbox->Unlock();

    // Destroy the message, but only after we've popped it from the queue.
    MessageCreator::Destroy(message_allocator, message);
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_SCHEDULER_MAILBOXPROCESSOR_H
