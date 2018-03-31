#ifndef AF_DETAIL_HANDLERS_DEFAULTHANDLERCOLLECTION_H
#define AF_DETAIL_HANDLERS_DEFAULTHANDLERCOLLECTION_H


#include <new>

#include "AF/address.h"
#include "AF/allocator_interface.h"
#include "AF/allocator_manager.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/handlers/blind_default_handler.h"
#include "AF/detail/handlers/default_handler.h"
#include "AF/detail/handlers/default_handler_interface.h"

#include "AF/detail/messages/message_interface.h"


namespace AF
{

class Actor;

namespace Detail
{

/*
 * A collection of default message handlers.
 * The 'collection' only holds one handler.
 */
class DefaultHandlerCollection
{
public:
    DefaultHandlerCollection();

    ~DefaultHandlerCollection();

    template <class ActorType>
    inline bool Set(void (ActorType::*handler)(const Address from));

    template <class ActorType>
    inline bool Set(void (ActorType::*handler)(const void *const data, const uint32_t size, const Address from));

    bool Clear();

    bool Handle(Actor *const actor, const MessageInterface *const message);

private:
    DefaultHandlerCollection(const DefaultHandlerCollection &other);
    DefaultHandlerCollection &operator=(const DefaultHandlerCollection &other);

    // Updates the registered handlers with any changes made since the last message was processed.
    // This function is intentionally not force-inlined since the handlers don't usually change often.
    void UpdateHandlers();

    DefaultHandlerInterface *handler_;          // Currently registered handler (only one is supported).
    DefaultHandlerInterface *new_handler_;      // New registered handler (will replace the registered one on update).
    bool handlers_dirty_;                       // Flag indicating that the handlers are out of date.
};


template <class ActorType>
inline bool DefaultHandlerCollection::Set(void (ActorType::*handler)(const Address from)) {
    typedef DefaultHandler<ActorType> MessageHandlerType;

    AllocatorInterface *const allocator(AllocatorManager::GetCache());

    handlers_dirty_ = true;

    // Destroy any previously set new handler.
    // Note that all handler objects are the same size.
    if (new_handler_) {
        new_handler_->~DefaultHandlerInterface();
        allocator->FreeWithSize(new_handler_, sizeof(MessageHandlerType));
        new_handler_ = 0;
    }

    if (handler) {
        // Allocate memory for a message handler object.
        void *const memory = allocator->Allocate(sizeof(MessageHandlerType));
        if (memory == 0) {
            return false;
        }

        // Construct a handler object to remember the function pointer and message value type.
        new_handler_ = new (memory) MessageHandlerType(handler);
    }

    return true;
}

template <class ActorType>
inline bool DefaultHandlerCollection::Set(void (ActorType::*handler)(const void *const data, const uint32_t size, const Address from)) {
    typedef BlindDefaultHandler<ActorType> MessageHandlerType;

    AllocatorInterface *const allocator(AllocatorManager::GetCache());

    handlers_dirty_ = true;

    // Destroy any previously set new handler.
    // Note that all handler objects are the same size.
    if (new_handler_) {
        new_handler_->~DefaultHandlerInterface();
        allocator->FreeWithSize(new_handler_, sizeof(MessageHandlerType));
        new_handler_ = 0;
    }

    if (handler) {
        // Allocate memory for a message handler object.
        void *const memory = allocator->Allocate(sizeof(MessageHandlerType));
        if (memory == 0) {
            return false;
        }

        // Construct a handler object to remember the function pointer and message value type.
        new_handler_ = new (memory) MessageHandlerType(handler);
    }

    return true;
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_HANDLERS_DEFAULTHANDLERCOLLECTION_H
