#ifndef AF_DETAIL_HANDLERS_HANDLERCOLLECTION_H
#define AF_DETAIL_HANDLERS_HANDLERCOLLECTION_H


#include <new>

#include "AF/address.h"
#include "AF/allocator_interface.h"
#include "AF/allocator_manager.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/containers/list.h"

#include "AF/detail/handlers/message_handler.h"
#include "AF/detail/handlers/message_handler_interface.h"
#include "AF/detail/handlers/message_handler_cast.h"

#include "AF/detail/messages/message_interface.h"
#include "AF/detail/messages/message_traits.h"

#include "AF/detail/scheduler/mailbox_context.h"
#include "AF/detail/scheduler/scheduler_interface.h"


namespace AF
{

class Actor;

namespace Detail
{

class HandlerCollection {
public:
    HandlerCollection();

    ~HandlerCollection();

    template <class ActorType, class ValueType>
    inline bool Add(void (ActorType::*handler)(const ValueType &message, const Address from));

    template <class ActorType, class ValueType>
    inline bool Remove(void (ActorType::*handler)(const ValueType &message, const Address from));

    template <class ActorType, class ValueType>
    inline bool Contains(void (ActorType::*handler)(const ValueType &message, const Address from)) const;

    inline bool Clear();

    inline bool Handle(
        MailboxContext *const mailbox_context,
        Actor *const actor,
        const MessageInterface *const message);

private:
    typedef List<MessageHandlerInterface> MessageHandlerList;

    HandlerCollection(const HandlerCollection &other);
    HandlerCollection &operator=(const HandlerCollection &other);

    void UpdateHandlers();

    MessageHandlerList handlers_;       // List of handlers in the collection.
    MessageHandlerList new_handlers_;   // List of handlers added since last update.
    bool handlers_dirty_;               ///< Flag indicating that the handlers are out of date.
};


template <class ActorType, class ValueType>
AF_FORCEINLINE bool HandlerCollection::Add(void (ActorType::*handler)(const ValueType &message, const Address from)) {
    typedef MessageHandler<ActorType, ValueType> MessageHandlerType;

    AllocatorInterface *const allocator(AllocatorManager::GetCache());

    // Allocate memory for a message handler object.
    void *const memory = allocator->Allocate(sizeof(MessageHandlerType));
    if (memory == 0) {
        return false;
    }

    // Construct a handler object to remember the function pointer and message value type.
    MessageHandlerType *const message_handler = new (memory) MessageHandlerType(handler);

    // We don't check for duplicates because multiple registrations of the same function are allowed.
    new_handlers_.Insert(message_handler);
    handlers_dirty_ = true;

    return true;
}

template <class ActorType, class ValueType>
AF_FORCEINLINE bool HandlerCollection::Remove(void (ActorType::*handler)(const ValueType &message, const Address from)) {
    // If the message value type has a valid (non-zero) type name defined for it,
    // then we use explicit type names to match messages to handlers.
    // The default value of zero indicates that no type name has been defined,
    // in which case we rely on compiler-generated RTTI to identify message types.
    typedef MessageHandler<ActorType, ValueType> MessageHandlerType;
    typedef MessageHandlerCast<ActorType, MessageTraits<ValueType>::HAS_TYPE_NAME> HandlerCaster;

    // We don't need to lock this because only one thread can access it at a time.
    // Find the handler in the registered handler list.
    typename MessageHandlerList::Iterator handlers(handlers_.GetIterator());
    while (handlers.Next()) {
        MessageHandlerInterface *const message_handler(handlers.Get());

        // Try to convert this handler, of unknown type, to the target type.
        if (const MessageHandlerType *const typed_handler = HandlerCaster:: template CastHandler<ValueType>(message_handler)) {
            // Don't count the handler if it's already marked for deregistration.
            if (typed_handler->GetHandlerFunction() == handler && !typed_handler->IsMarked()) {
                // Mark the handler for deregistration.
                // We defer the actual deregistration and deletion until
                // after all active message handlers have been executed, because
                // message handlers themselves can deregister handlers.
                message_handler->Mark();
                handlers_dirty_ = true;

                return true;
            }
        }
    }

    // The handler wasn't in the registered list, but maybe it's in the new handlers list.
    // That can happen if the handler was only just registered prior to this in the same function.
    // It's a bit weird to register a handler and then immediately deregister it, but legal.
    handlers = new_handlers_.GetIterator();
    while (handlers.Next()) {
        MessageHandlerInterface *const message_handler(handlers.Get());

        // Try to convert this handler, of unknown type, to the target type.
        if (const MessageHandlerType *const typed_handler = HandlerCaster:: template CastHandler<ValueType>(message_handler)) {
            // Don't count the handler if it's already marked for deregistration.
            if (typed_handler->GetHandlerFunction() == handler && !typed_handler->IsMarked()) {
                // Mark the handler for deregistration.
                message_handler->Mark();
                handlers_dirty_ = true;

                return true;
            }
        }
    }

    return false;
}

template <class ActorType, class ValueType>
AF_FORCEINLINE bool HandlerCollection::Contains(void (ActorType::*handler)(const ValueType &message, const Address from)) const {
    typedef MessageHandler<ActorType, ValueType> MessageHandlerType;
    typedef MessageHandlerCast<ActorType, MessageTraits<ValueType>::HAS_TYPE_NAME> HandlerCaster;

    // Search for the handler in the registered handler list.
    typename MessageHandlerList::Iterator handlers(handlers_.GetIterator());
    while (handlers.Next()) {
        MessageHandlerInterface *const message_handler(handlers.Get());

        // Try to convert this handler, of unknown type, to the target type.
        if (const MessageHandlerType *const typed_handler = HandlerCaster:: template CastHandler<ValueType>(message_handler)) {
            // Count as not registered if it's marked for deregistration.
            // But it may be registered more than once, so keep looking.
            if (typed_handler->GetHandlerFunction() == handler && !typed_handler->IsMarked()) {
                return true;
            }
        }
    }

    // The handler wasn't in the registered list, but maybe it's in the new handlers list.
    handlers = new_handlers_.GetIterator();
    while (handlers.Next()) {
        MessageHandlerInterface *const message_handler(handlers.Get());

        // Try to convert this handler, of unknown type, to the target type.
        if (const MessageHandlerType *const typed_handler = HandlerCaster:: template CastHandler<ValueType>(message_handler)) {
            // Count as not registered if it's marked for deregistration.
            // But it may be registered more than once, so keep looking.
            if (typed_handler->GetHandlerFunction() == handler && !typed_handler->IsMarked()) {
                return true;
            }
        }
    }

    return false;
}

AF_FORCEINLINE bool HandlerCollection::Clear() {
    AllocatorInterface *const allocator(AllocatorManager::GetCache());

    // Free all currently allocated handler objects.
    while (MessageHandlerInterface *const handler = handlers_.Front()) {
        handlers_.Remove(handler);
        handler->~MessageHandlerInterface();
        allocator->Free(handler);
    }

    while (MessageHandlerInterface *const handler = new_handlers_.Front()) {
        new_handlers_.Remove(handler);
        handler->~MessageHandlerInterface();
        allocator->Free(handler);
    }

    handlers_dirty_ = false;
    return true;
}

AF_FORCEINLINE bool HandlerCollection::Handle(
    MailboxContext *const mailbox_context,
    Actor *const actor,
    const MessageInterface *const message) {
    bool handled(false);
    SchedulerInterface *const scheduler(mailbox_context->scheduler_);

    AF_ASSERT(scheduler);
    AF_ASSERT(actor);
    AF_ASSERT(message);

    // Update the handler list if there have been changes.
    if (handlers_dirty_) {
        UpdateHandlers();
    }

    // Give each registered handler a chance to handle this message.
    MessageHandlerList::Iterator handlers(handlers_.GetIterator());
    while (handlers.Next()) {
        MessageHandlerInterface *const message_handler(handlers.Get());

        // We notify the scheduler, which acts as an observer.
        scheduler->BeginHandler(mailbox_context, message_handler);
        handled |= message_handler->Handle(actor, message);
        scheduler->EndHandler(mailbox_context, message_handler);
    }

    return handled;
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_HANDLERS_HANDLERCOLLECTION_H
