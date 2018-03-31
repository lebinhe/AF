#ifndef AF_ACTOR_H
#define AF_ACTOR_H


#include "AF/address.h"
#include "AF/align.h"
#include "AF/allocator_interface.h"
#include "AF/allocator_manager.h"
#include "AF/basic_types.h"
#include "AF/defines.h"
#include "AF/framework.h"

#include "AF/detail/directory/directory.h"

#include "AF/detail/handlers/default_handler_collection.h"
#include "AF/detail/handlers/fallback_handler_collection.h"
#include "AF/detail/handlers/handler_collection.h"

#include "AF/detail/mailboxes/mailbox.h"

#include "AF/detail/messages/message_interface.h"
#include "AF/detail/messages/message_creator.h"

#include "AF/detail/scheduler/mailbox_context.h"

#include "AF/detail/threading/atomic.h"
#include "AF/detail/utils/utils.h"


namespace AF
{

class Framework;

namespace Detail {
    class MailboxProcessor;
}


/*
 * The actor baseclass.
 */
class Actor {
public:

    friend class Framework;
    friend class Detail::MailboxProcessor;

    explicit Actor(Framework &framework, const char *const name = 0);

    virtual ~Actor();

    inline Address GetAddress() const;

    inline Framework &GetFramework() const;

    inline uint32_t GetNumQueuedMessages() const;

protected:

    template <class ActorType, class ValueType>
    inline bool RegisterHandler(
        ActorType *const actor,
        void (ActorType::*handler)(const ValueType &message, const Address from));

    template <class ActorType, class ValueType>
    inline bool DeregisterHandler(
        ActorType *const actor,
        void (ActorType::*handler)(const ValueType &message, const Address from));

    template <class ActorType, class ValueType>
    inline bool IsHandlerRegistered(
        ActorType *const actor,
        void (ActorType::*handler)(const ValueType &message, const Address from));

    template <class ActorType>
    inline bool SetDefaultHandler(
        ActorType *const actor,
        void (ActorType::*handler)(const Address from));

    template <class ActorType>
    inline bool SetDefaultHandler(
        ActorType *const actor,
        void (ActorType::*handler)(const void *const data, const uint32_t size, const Address from));

    template <class ValueType>
    inline bool Send(const ValueType &value, const Address &address) const;

private:

    // Actors are non-copyable.
    Actor(const Actor &other);
    Actor &operator=(const Actor &other);

    inline void ProcessMessage(
        Detail::MailboxContext *const mailbox_context,
        Detail::FallbackHandlerCollection *const fallback_handlers,
        Detail::MessageInterface *const message);

    void Fallback(
        Detail::FallbackHandlerCollection *const fallback_handlers,
        const Detail::MessageInterface *const message);

    Address address_;                                   // Unique address of this actor.
    Framework *framework_;                              // Pointer to the framework within which the actor runs.
    Detail::HandlerCollection message_handlers_;        // The message handlers registered by this actor.
    Detail::DefaultHandlerCollection default_handlers_; // Default message handlers registered by this actor.
    Detail::MailboxContext *mailbox_context_;           // Remembers the context of the worker thread processing the actor.

    void *memory_;                                      // Pointer to memory block containing final actor type.
};


inline Address Actor::GetAddress() const {
    return address_;
}

AF_FORCEINLINE Framework &Actor::GetFramework() const {
    return *framework_;
}

AF_FORCEINLINE uint32_t Actor::GetNumQueuedMessages() const {
    const Address address(GetAddress());
    Framework &framework(GetFramework());
    const Detail::Mailbox &mailbox(framework.mailboxes_.GetEntry(address.AsInteger()));

    return mailbox.Count();
}

template <class ActorType, class ValueType>
inline bool Actor::RegisterHandler(
    ActorType *const actor,
    void (ActorType::*handler)(const ValueType &message, const Address from)) {

    return message_handlers_.Add(handler);
}

template <class ActorType, class ValueType>
inline bool Actor::DeregisterHandler(
    ActorType *const actor,
    void (ActorType::*handler)(const ValueType &message, const Address from)) {
    return message_handlers_.Remove(handler);
}

template <class ActorType, class ValueType>
inline bool Actor::IsHandlerRegistered(
    ActorType *const actor,
    void (ActorType::*handler)(const ValueType &message, const Address from)) {
    return message_handlers_.Contains(handler);
}

template <class ActorType>
inline bool Actor::SetDefaultHandler(
    ActorType *const actor,
    void (ActorType::*handler)(const Address from)) {
    return default_handlers_.Set(handler);
}

template <class ActorType>
inline bool Actor::SetDefaultHandler(
    ActorType *const actor,
    void (ActorType::*handler)(const void *const data, const uint32_t size, const Address from)) {
    return default_handlers_.Set(handler);
}

template <class ValueType>
AF_FORCEINLINE bool Actor::Send(const ValueType &value, const Address &address) const {
    // Try to use the processor context owned by a worker thread.
    // The current thread will be a worker thread if this method has been called from a message
    // handler. If it was called from an actor constructor or destructor then the current thread
    // may be an application thread, in which case the stored context pointer will be null.
    // If it is null we fall back to the per-framework context, which is shared between threads.
    // The advantage of using a thread-specific context is that it is only accessed by that
    // single thread so doesn't need to be thread-safe and isn't written by other threads
    // so doesn't cause expensive cache coherency updates between cores.
    Detail::MailboxContext *mailbox_context(mailbox_context_);
    if (mailbox_context_ == 0) {
        mailbox_context = framework_->GetMailboxContext();
    }

    // Allocate a message. It'll be deleted by the worker thread that handles it.
    Detail::MessageInterface *const message(Detail::MessageCreator::Create(
        mailbox_context->message_allocator_,
        value,
        address_));

    if (message) {
        // Call the message sending implementation using the acquired processor context.
        return framework_->SendInternal(
            mailbox_context,
            message,
            address);
    }

    return false;
}

AF_FORCEINLINE void Actor::ProcessMessage(
    Detail::MailboxContext *const mailbox_context,
    Detail::FallbackHandlerCollection *const fallback_handlers,
    Detail::MessageInterface *const message) {
    // Store a pointer to the context data for this thread in the actor.
    // We'll need it to send messages if any of the registered handlers
    // call Actor::Send, but we can't pass it through from here because
    // the handlers are user code.
    AF_ASSERT(mailbox_context_ == 0);
    mailbox_context_ = mailbox_context;

    if (!message_handlers_.Handle(mailbox_context, this, message)) {
        // If no registered handler handled the message, execute the default handlers instead.
        // This call is intentionally not inlined to avoid polluting the generated code with the uncommon case.
        Fallback(fallback_handlers, message);
    }

    // Zero the context pointer, in case it's next accessed by a non-worker thread.
    AF_ASSERT(mailbox_context_ == mailbox_context);
    mailbox_context_ = 0;
}


} // namespace AF


#endif // AF_ACTOR_H

